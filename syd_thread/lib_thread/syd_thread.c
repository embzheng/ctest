/* Copyright (C) 2001-2003 IP Infusion, Inc. All Rights Reserved. */

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/poll.h>

//#include <libpub/syd_thread/syd_thread.h>
#include "syd_thread.h"
#include "lib.h"
#include "timeutil.h"

#define LIB_PARAM_CHECK 1

#define syd_thread_gettid() syscall(__NR_gettid)


/*
   Thread.c maintains a list of all the "callbacks" waiting to run.

   It is RTOS configurable with "HAVE_RTOS_TIC", "HAVE_RTOS_TIMER",
   "RTOS_EXECUTE_ONE_THREAD".

   For Linux/Unix, all the above are undefined; the main task will
   just loop continuously on "thread_fetch" and "thread_call".
   "thread_fetch" will stay in a tight loop until all active THREADS
   are run.

   When "HAVE_RTOS_TIC" is defined, the RTOS is expected to emulate
   the "select" function as non-blocking, and call "lib_tic" for any
   I/O ready (by setting PAL_SOCK_HANDLESET_ISSET true), and must call
   "lib_tic" at least once every 1 second.

   When "HAVE_RTOS_TIMER" is defined, the RTOS must support the
   "rtos_set_timer( )", and set a timer; when it expires, it must call
   "lib_tic ( )".

   When "RTOS_EXECUTE_ONE_THREAD" is defined, the RTOS must treat
   ZebOS as strict "BACKGROUND".  The BACKGROUND MANAGER Keeps track
   of all ZebOS Routers, and calls them in strict sequence.  When
   "lib_tic" is called, ONLY ONE THREAD will be run, and a TRUE state
   will be SET to indicate that a THREAD has RUN, so the BACKGROUND
   should give control to the FOREGROUND once again.  If not SET, the
   BACKGROUND is allowed to go onto another Router to try its THREAD
   and so-forth, through all routers.
*/

/* Allocate new thread master.  */
struct syd_thread_master *
syd_thread_master_create (void)
{
  struct syd_thread_master *master;

  master = XCALLOC (MTYPE_THREAD_MASTER, sizeof (struct syd_thread_master));
  if (master != NULL)
    {
      SYD_THREAD_SET_FLAG(master->flag, SYD_THREAD_IS_FIRST_FETCH); //SYD_THREAD_IS_FIRST_FETCH = 1
    }
  
  return master;
}

int
syd_thread_use_poll (struct syd_thread_master *m, short max_fds)
{
  int i;

  if (m == NULL)
    {
      return -1;
    }

  SYD_THREAD_INTERFACE_DBG (m, "User set use poll, max fd num %d.\n", max_fds);

  m->pfds = XMALLOC (MTYPE_POLLFD, max_fds * sizeof (struct pollfd));
  if (m->pfds == NULL)
    {
      SYD_THREAD_INTERFACE_DBG (m, "Failed to create pollfd array(%d).\n", max_fds);
      return -1;
    }

  m->nfds = 0;  /* 初始化时，认为有效的pollfd数组为0 */
  m->pfds_size = max_fds;
  SYD_THREAD_SET_FLAG (m->flag, SYD_THREAD_USE_POLL);
  /* 初始化pollfd数组的内容 */
  for (i = 0; i < max_fds; i++)
    {
      m->pfds[i].fd = -1;
      m->pfds[i].events = 0;
    }

  return 0;
}

/*
  fill up pollfd with file descriptable
  and event
*/

static void
syd_thread_set_pollfd (struct syd_thread *thread, uint32_t events)
{
  int i, index;

  index = SYD_THREAD_INVALID_INDEX; /* 初始化为-1表示不可用 */
  /*
   * thread->pfd_index保存的是上一次分配给它的索引，
   * 一般情况下进程模块都会马上再次添加该伪线程，此时取到这个index就能再次分配到这个索引
   */
   
  for (i = thread->pfd_index; i < thread->master->pfds_size; i++)
    {
      if (thread->master->pfds[i].fd < 0)
        {
          /* 找到一个可用的数组元素 */
          index = i;
          break;
        }
    }

  if (index == SYD_THREAD_INVALID_INDEX)
    {
      /* 从前半段继续尝试查找 */
      for (i = 0; i < thread->pfd_index; i++)
        if (thread->master->pfds[i].fd < 0)
          {
            /* 找到一个可用的数组元素 */
            index = i;
            break;
          }

      if (index == SYD_THREAD_INVALID_INDEX)
        {
          /*
           * 找不到可用的数组元素，设置失败
           * 因为已经由用户设置最大fd数目了，如果不够，是使用模块创建的fd已经超出了总量
           */
          SYD_THREAD_PROCEDURE_DBG (thread->master, "Failed to find array index.\n");
          assert(0);
          return;
        }
    }

  thread->pfd_index = index;  /* 将所分配到的索引保存起来 */
  thread->master->pfds[index].fd = SYD_THREAD_FD (thread);
  thread->master->pfds[index].events = events;
  thread->master->pfds[index].revents = 0;
  if (index > thread->master->nfds)
    {
      /* 重新设置pollfd数组的个数 */
      thread->master->nfds = index;
      SYD_THREAD_PROCEDURE_DBG (thread->master, "Max nfds %d.\n", thread->master->nfds);
    }
  return;
}

static inline void
syd_thread_clear_pollfd (struct syd_thread *thread)
{
  thread->master->pfds[thread->pfd_index].fd = -1;
  thread->master->pfds[thread->pfd_index].events = 0;

  return;
}

/* Add a new thread to the list.  */
void
syd_thread_list_add (struct syd_thread_list *list, struct syd_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
  assert(thread != NULL);
#endif

  thread->next = NULL;
  thread->prev = list->tail;
  if (list->tail)
    list->tail->next = thread;
  else
    list->head = thread;
  list->tail = thread;
  list->count++;
}

/* Add a new thread just after the point. If point is NULL, add to top. */
static void
syd_thread_list_add_after (struct syd_thread_list *list,
                       struct syd_thread *point,
                       struct syd_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
  assert(thread != NULL);
#endif

  thread->prev = point;
  if (point)
    {
      if (point->next)
        point->next->prev = thread;
      else
        list->tail = thread;
      thread->next = point->next;
      point->next = thread;
    }
  else
    {
      if (list->head)
        list->head->prev = thread;
      else
        list->tail = thread;
      thread->next = list->head;
      list->head = thread;
    }
  list->count++;
}

/* Delete a thread from the list. */
static struct syd_thread *
syd_thread_list_delete (struct syd_thread_list *list, struct syd_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
  assert(thread != NULL);
  if (list != &thread->master->unuse)
    assert(thread->type != SYD_THREAD_UNUSED);
#endif

  if (thread->next)
    thread->next->prev = thread->prev;
  else
    list->tail = thread->prev;
  if (thread->prev)
    thread->prev->next = thread->next;
  else
    list->head = thread->next;
  thread->next = thread->prev = NULL;
  list->count--;
  return thread;
}

/* Delete top of the list and return it. */
static struct syd_thread *
syd_thread_trim_head (struct syd_thread_list *list)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  if (list->head)
    return syd_thread_list_delete (list, list->head);
  return NULL;
}

/* Free half unused thread. */
static void
syd_thread_unuse_list_free_half (struct syd_thread_master *m)
{
  int i;
  struct syd_thread *t;
  struct syd_thread *next;
  struct syd_thread_list *list;

  assert (m != NULL);
  list = &(m->unuse);
  assert (list != NULL);

  for (t = list->head, i = SYD_THREAD_FREE_UNUSE_MAX; t && i; t = next, i--)
    {
      next = t->next;
      syd_thread_list_delete(list, t);
      XFREE (MTYPE_THREAD, t);
    }

  assert (t != NULL && i == 0);

  m->alloc -= SYD_THREAD_FREE_UNUSE_MAX;

  return;
}

/* Move thread to unuse list. */
static void
syd_thread_add_unuse (struct syd_thread_master *m, struct syd_thread *thread)
{
  assert (m != NULL);
  assert (thread->next == NULL);
  assert (thread->prev == NULL);
  assert (thread->type == SYD_THREAD_UNUSED);
  syd_thread_list_add (&m->unuse, thread);

  if (m->unuse.count >= SYD_THREAD_UNUSE_MAX)
    syd_thread_unuse_list_free_half (m);

  return;
}

/* Unuse thread */
static void
syd_thread_unuse (struct syd_thread_master *m, struct syd_thread *thread)
{
  thread->type = SYD_THREAD_UNUSED;
  SYD_THREAD_UNSET_FLAG(thread->flag, SYD_THREAD_FLAG_IN_LIST);
  if (!SYD_THREAD_CHECK_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR) && thread != m->t_recover)
    syd_thread_add_unuse (m, thread);
}

/* Free all unused thread. */
static void
syd_thread_list_free (struct syd_thread_master *m, struct syd_thread_list *list)
{
  struct syd_thread *t;
  struct syd_thread *next;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
#endif

  for (t = list->head; t; t = next)
    {
      next = t->next;
      /*
       * 如果是用户自己创建的伪线程，那么不用删除，还放在链表中。
       * 不用担心链表还保留着这些伪线程，因为master很快就被删除掉，也就没有链表了。
       * 需要把该伪线程设置为不在链表中，用户无需再cancel，只需删除它即可。
       */
      if (SYD_THREAD_CHECK_FLAG(t->flag, SYD_THREAD_FLAG_CRT_BY_USR))
        {
          SYD_THREAD_UNSET_FLAG(t->flag, SYD_THREAD_FLAG_IN_LIST);
          continue;
        }
      XFREE (MTYPE_THREAD, t);
      list->count--;
      m->alloc--;
    }
}

void
syd_thread_list_execute (struct syd_thread_master *m, struct syd_thread_list *list)
{
  struct syd_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
#endif

  thread = syd_thread_trim_head (list);
  if (thread != NULL)
    {
      syd_thread_execute (thread->func, thread->arg, thread->u.val);
      syd_thread_unuse (m, thread);
    }
}

void
syd_thread_list_clear (struct syd_thread_master *m, struct syd_thread_list *list)
{
  struct syd_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
#endif

  while ((thread = syd_thread_trim_head (list)))
    {
      syd_thread_unuse (m, thread);
    }
}

/* Stop thread scheduler. */
void
syd_thread_master_finish (struct syd_thread_master *m)
{
  int i;

  if (m == NULL)
    {
     return;
    }

  syd_thread_list_free (m, &m->queue_high);
  syd_thread_list_free (m, &m->queue_middle);
  syd_thread_list_free (m, &m->queue_low);
  syd_thread_list_free (m, &m->read);
  syd_thread_list_free (m, &m->read_high);
  syd_thread_list_free (m, &m->write);
  for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
    syd_thread_list_free (m, &m->timer[i]);
  syd_thread_list_free (m, &m->event);
  syd_thread_list_free (m, &m->event_low);
  syd_thread_list_free (m, &m->unuse);

  if (m->pfds)
    XFREE (MTYPE_POLLFD, m->pfds);

  XFREE (MTYPE_THREAD_MASTER, m);
}

/* Thread list is empty or not.  */
int
syd_thread_empty (struct syd_thread_list *list)
{
#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  return  list->head ? 0 : 1;
}

/* Return remain time in second. */
u_int32_t
syd_thread_timer_remain_second (struct syd_thread *thread)
{
  struct timeval timer_now;

  if (thread == NULL)
    return 0;

  syd_time_tzcurrent (&timer_now, NULL);

  if (thread->u.sands.tv_sec - timer_now.tv_sec > 0)
    return thread->u.sands.tv_sec - timer_now.tv_sec;
  else
    return 0;
}

/* Get new thread.  */
struct syd_thread *
syd_thread_get (struct syd_thread_master *m, char type,
            int (*func) (struct syd_thread *), void *arg)
{
  struct syd_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  if (m->unuse.head)
    thread = syd_thread_trim_head (&m->unuse);
  else
    {
      thread = XMALLOC (MTYPE_THREAD, sizeof (struct syd_thread));
      if (thread == NULL)
        {
          SYD_THREAD_INTERFACE_DBG (m, "Failed to malloc memory for syd-thread.\n");
          return NULL;
        }

      m->alloc++;
    }
  thread->type = type;
  thread->master = m;
  thread->func = func;
  thread->arg = arg;
  thread->pfd_index = 0;
  memset (thread->name, 0, sizeof (thread->name));
  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_IN_LIST);

  return thread;
}

static int
syd_thread_name_check (struct syd_thread_master *m, char *name)
{
  assert (name);

  /* 如果名字过长，只是调试信息提醒，后续保存时只截取10个字符 */
  if (strlen (name) > SYD_THREAD_NAME_LEN)
    {
      SYD_THREAD_INTERFACE_DBG (m, "syd_thread name length exceeded %d.\n",
                                       SYD_THREAD_NAME_LEN);
    }

  return 0;
}

static int
syd_thread_fd_check (struct syd_thread_master *m, int fd)
{
  if (fd < 0)
    {
      SYD_THREAD_INTERFACE_DBG (m, "Invalid fd %d.\n", fd);
      return -1;
    }

  if (fd > SYD_THREAD_SELECT_FD_MAX)
    {
      /* 如果包含超过1024的fd，那么必须使用poll机制 */
      assert (SYD_THREAD_CHECK_FLAG(m->flag, SYD_THREAD_USE_POLL));
      if (!SYD_THREAD_CHECK_FLAG(m->flag, SYD_THREAD_USE_POLL))
        {
          SYD_THREAD_PRINT ("THREAD ERROR fd > 1024, %d\n", fd);
          return -1;
        }
    }

  return 0;
}

static void
syd_thread_name_add (struct syd_thread *thread, char *name)
{
  memset (thread->name, 0, sizeof(thread->name));
  if (name != NULL)
    strncpy (thread->name, name, SYD_THREAD_NAME_LEN);

  return;
}

static inline void
syd_thread_add_thread_check(struct syd_thread_master *m, struct syd_thread *thread)
{
  int tid;

  if (!SYD_THREAD_DEBUG_INTERFACE_ON(m))
      return;

  SYD_THREAD_PRINT("Check thread %p.\n", thread);

  tid = syd_thread_gettid();
  if (tid != m->tid && m->tid != 0)
    {
      SYD_THREAD_PRINT ("Error: syd-thread %p is added from another p-thread, [%d->%d].\n",
              thread, tid, m->tid);
    }

  return;
}

static inline int
syd_thread_insert_thread_arg_check(struct syd_thread_master *m, struct syd_thread *thread, char *name)
{
  if (m == NULL)
    {
      return -1;
    }

  if (thread == NULL)
    {
      SYD_THREAD_INTERFACE_DBG (m, "Insert new thread failed(thread arg null).\n");
      return -1;
    }

  syd_thread_add_thread_check(m, thread);

  if (name != NULL && syd_thread_name_check (m, name))
    return -1;

  return 0;
}

/* Insert new thread.  */
int
syd_thread_insert (struct syd_thread_master *m, struct syd_thread *thread, char type,
            int (*func) (struct syd_thread *), void *arg, char *name)
{
  thread->type = type;
  thread->master = m;
  thread->func = func;
  thread->arg = arg;

  syd_thread_name_add (thread, name);
  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_IN_LIST);

  return 0;
}

/* Keep track of the maximum file descriptor for read/write. */
static void
syd_thread_update_max_fd (struct syd_thread_master *m, int fd)
{
  if (m && m->max_fd < fd)
    m->max_fd = fd;
}

static inline void
syd_thread_update_read (struct syd_thread_master *m, struct syd_thread *thread, int fd)
{
  thread->u.fd = fd;  /* 必须先设置，在下面的函数中会用到该值 */

  if (SYD_THREAD_CHECK_FLAG(m->flag, SYD_THREAD_USE_POLL))
    {
      syd_thread_set_pollfd (thread, POLLHUP | POLLIN);/*  普通可读或挂起  */
    }
  else
    {
      syd_thread_update_max_fd (m, fd);
      FD_SET (fd, &m->readfd);
    }
}

/* Add new read thread. */
struct syd_thread *
syd_thread_add_read (struct syd_thread_master *m,
                    int (*func) (struct syd_thread *),
                    void *arg, int fd)
{
  struct syd_thread *thread;

  assert (m != NULL);

  if (syd_thread_fd_check (m, fd) < 0)
    {
      return NULL;
    }

  SYD_THREAD_INTERFACE_DBG (m, "Add new read thread fd %d, func %p.\n", fd, func);

  thread = syd_thread_get (m, SYD_THREAD_READ, func, arg);
  if (thread == NULL)
    return NULL;

  syd_thread_add_thread_check(m, thread);

  syd_thread_update_read(m, thread, fd);
  syd_thread_list_add (&m->read, thread);

  return thread;
}

/* Add new read thread with name. */
struct syd_thread *
syd_thread_add_read_withname (struct syd_thread_master *m,
                             int (*func) (struct syd_thread *),
                             void *arg, int fd, char *name)
{
  struct syd_thread *thread;

  if (name == NULL)
    return syd_thread_add_read (m, func, arg, fd);

  if (syd_thread_name_check (m, name))
    return NULL;

  thread = syd_thread_add_read (m, func, arg, fd);
  if (thread)
    syd_thread_name_add (thread, name);

  return thread;
}

/* Add new high priority read thread. */
struct syd_thread *
syd_thread_add_read_high (struct syd_thread_master *m,
                         int (*func) (struct syd_thread *),
                         void *arg, int fd)
{
  struct syd_thread *thread;

  assert (m != NULL);

  if (syd_thread_fd_check (m, fd) < 0)
    {
      return NULL;
    }

  SYD_THREAD_INTERFACE_DBG (m, "Add new read high thread fd %d, func %p.\n", fd, func);

  thread = syd_thread_get (m, SYD_THREAD_READ_HIGH, func, arg);
  if (thread == NULL)
    return NULL;

  syd_thread_add_thread_check(m, thread);

  syd_thread_update_read(m, thread, fd);
  syd_thread_list_add (&m->read_high, thread);

  return thread;
}

/* Add new high priority read thread with name. */
struct syd_thread *
syd_thread_add_read_high_withname (struct syd_thread_master *m,
                                  int (*func) (struct syd_thread *),
                                  void *arg, int fd, char *name)
{
  struct syd_thread *thread;

  if (name == NULL)
    return syd_thread_add_read_high (m, func, arg, fd);

  if (syd_thread_name_check (m, name))
    return NULL;

  thread = syd_thread_add_read_high (m, func, arg, fd);
  if (thread)
    syd_thread_name_add (thread, name);

  return thread;
}

/* Insert new read thread. */
int
syd_thread_insert_read (struct syd_thread_master *m, struct syd_thread *thread,
                             int (*func) (struct syd_thread *),
                             void *arg, int fd, char *name)
{
  if (syd_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  if (syd_thread_fd_check (m, fd) < 0)
    {
      return -1;
    }

  SYD_THREAD_INTERFACE_DBG (m, "Insert new read thread fd %d, func %p.\n", fd, func);

  syd_thread_insert(m, thread, SYD_THREAD_READ, func, arg, name);
  syd_thread_update_read(m, thread, fd);
  syd_thread_list_add (&m->read, thread);
  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Insert new high priority read thread. */
int
syd_thread_insert_read_high (struct syd_thread_master *m, struct syd_thread *thread,
                         int (*func) (struct syd_thread *),
                         void *arg, int fd, char *name)
{
  if (syd_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  if (syd_thread_fd_check (m, fd) < 0)
    {
      return -1;
    }

  SYD_THREAD_INTERFACE_DBG (m, "Insert new read high thread fd %d, func %p, %s.\n", fd, func, name);

  syd_thread_insert(m, thread, SYD_THREAD_READ_HIGH, func, arg, name);
  syd_thread_update_read(m, thread, fd);
  syd_thread_list_add (&m->read_high, thread);
  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

static inline void
syd_thread_update_write (struct syd_thread_master *m, struct syd_thread *thread, int fd)
{
  thread->u.fd = fd;  /* 必须先设置，在下面的函数中会用到该值 */

  if (SYD_THREAD_CHECK_FLAG(m->flag, SYD_THREAD_USE_POLL))
    {
      syd_thread_set_pollfd (thread, POLLHUP | POLLOUT);
    }
  else
    {
      syd_thread_update_max_fd (m, fd);
      FD_SET (fd, &m->writefd);
    }

  syd_thread_list_add (&m->write, thread);
}

/* Add new write thread. */
struct syd_thread *
syd_thread_add_write (struct syd_thread_master *m,
                     int (*func) (struct syd_thread *),
                     void *arg, int fd)
{
  struct syd_thread *thread;

  assert (m != NULL);

  if (syd_thread_fd_check (m, fd) < 0 || FD_ISSET (fd, &m->writefd))
    {
      return NULL;
    }

  SYD_THREAD_INTERFACE_DBG (m, "Add new write thread fd %d, func %p.\n", fd, func);

  thread = syd_thread_get (m, SYD_THREAD_WRITE, func, arg);
  if (thread == NULL)
    return NULL;

  syd_thread_add_thread_check(m, thread);

  syd_thread_update_write(m, thread, fd);

  return thread;
}

/* Add new write read thread with name. */
struct syd_thread *
syd_thread_add_write_withname (struct syd_thread_master *m,
                              int (*func) (struct syd_thread *),
                              void *arg, int fd, char *name)
{
  struct syd_thread *thread;

  if (name == NULL)
    return syd_thread_add_write (m, func, arg, fd);

  if (syd_thread_name_check (m, name))
    return NULL;

  thread = syd_thread_add_write (m, func, arg, fd);
  if (thread)
    syd_thread_name_add (thread, name);

  return thread;
}

/* Insert new write thread. */
int
syd_thread_insert_write (struct syd_thread_master *m, struct syd_thread *thread,
                     int (*func) (struct syd_thread *),
                     void *arg, int fd, char *name)
{
  if (syd_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  if (syd_thread_fd_check (m, fd) < 0 || FD_ISSET (fd, &m->writefd))
    {
      return -1;
    }

  SYD_THREAD_INTERFACE_DBG (m, "Insert new write thread fd %d, func %p.\n", fd, func);

  syd_thread_insert(m, thread, SYD_THREAD_WRITE, func, arg, name);
  syd_thread_update_write(m, thread, fd);
  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

static void
syd_thread_add_timer_common (struct syd_thread_master *m, struct syd_thread *thread)
{
#ifndef TIMER_NO_SORT
  struct syd_thread *tt;
#endif /* TIMER_NO_SORT */

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(thread != NULL);
#endif

  /* Set index.  */
  thread->index = m->index;

  /* Sort by timeval. */
#ifdef TIMER_NO_SORT
  syd_thread_list_add (&m->timer[m->index], thread);
#else
  for (tt = m->timer[m->index].tail; tt; tt = tt->prev)
    if (timeval_cmp (thread->u.sands, tt->u.sands) >= 0)
      break;

  syd_thread_list_add_after (&m->timer[m->index], tt, thread);
#endif /* TIMER_NO_SORT */

  /* Increment timer slot index.  */
  m->index++;
  m->index %= SYD_THREAD_TIMER_SLOT;
}

static void
syd_thread_update_time(struct syd_thread * thread, long timer)
{
  struct timeval timer_now;

  syd_time_tzcurrent (&timer_now, NULL);
#ifdef HAVE_NGSA
  if (timer < 0)
    {
      timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
    }
  else
    {
      timer_now.tv_sec += timer;
      if (timer_now.tv_sec < 0)
        timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
    }
#else
  timer_now.tv_sec += timer;
#endif

  thread->u.sands = timer_now;
}

static void
syd_thread_update_timeval(struct syd_thread *thread, struct timeval *timer)
{
  struct timeval timer_now;

  /* Do we need jitter here? */
  syd_time_tzcurrent (&timer_now, NULL);
  timer_now.tv_sec += timer->tv_sec;
  timer_now.tv_usec += timer->tv_usec;
  while (timer_now.tv_usec >= TV_USEC_PER_SEC)
    {
      timer_now.tv_sec++;
      timer_now.tv_usec -= TV_USEC_PER_SEC;
    }

  /* Correct negative value.  */
  if (timer_now.tv_sec < 0)
    timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
  if (timer_now.tv_usec < 0)
    timer_now.tv_usec = PAL_TIME_MAX_TV_USEC;

  thread->u.sands = timer_now;
}

/* Add timer thread. */
struct syd_thread *
syd_thread_add_timer (struct syd_thread_master *m,
                     int (*func) (struct syd_thread *),
                     void *arg, long timer)
{
  struct syd_thread *thread;

  assert (m != NULL);
  thread = syd_thread_get (m, SYD_THREAD_TIMER, func, arg);
  if (thread == NULL)
    return NULL;

  syd_thread_add_thread_check(m, thread);

  syd_thread_update_time(thread, timer);

  /* Common process.  */
  syd_thread_add_timer_common (m, thread);

  return thread;
}

/* Add new timer thread with name. */
struct syd_thread *
syd_thread_add_timer_withname (struct syd_thread_master * m,
                              int(* func)(struct syd_thread *),
                              void * arg, long timer, char * name)
{
  struct syd_thread *thread;

  if (name == NULL)
    return syd_thread_add_timer (m, func, arg, timer);

  if (syd_thread_name_check (m, name))
    return NULL;

  thread = syd_thread_add_timer (m, func, arg, timer);
  if (thread)
    syd_thread_name_add (thread, name);

  return thread;
}

/* Add timer thread. */
struct syd_thread *
syd_thread_add_timer_timeval (struct syd_thread_master *m,
                             int (*func) (struct syd_thread *),
                             void *arg, struct timeval timer)
{
  struct syd_thread *thread;

  assert (m != NULL);

  thread = syd_thread_get (m, SYD_THREAD_TIMER, func, arg);
  if (thread == NULL)
    return NULL;

  syd_thread_add_thread_check(m, thread);

  syd_thread_update_timeval(thread, &timer);

  /* Common process.  */
  syd_thread_add_timer_common (m, thread);

  return thread;
}

/* Add new timerval thread with name. */
struct syd_thread *
syd_thread_add_timer_timeval_withname (struct syd_thread_master * m,
                                      int(* func)(struct syd_thread *),
                                      void * arg, struct timeval timer,
                                      char * name)
{
  struct syd_thread *thread;

  if (name == NULL)
    return syd_thread_add_timer_timeval (m, func, arg, timer);

  if (syd_thread_name_check (m, name))
    return NULL;

  thread = syd_thread_add_timer_timeval (m, func, arg, timer);
  if (thread)
    syd_thread_name_add (thread, name);

  return thread;
}

/* Insert timer thread. */
int
syd_thread_insert_timer (struct syd_thread_master *m, struct syd_thread *thread,
                     int (*func) (struct syd_thread *),
                     void *arg, long timer, char *name)
{
  if (syd_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  syd_thread_insert(m, thread, SYD_THREAD_TIMER, func, arg, name);
  syd_thread_update_time(thread, timer);

  /* Common process.  */
  syd_thread_add_timer_common (m, thread);

  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Insert timer thread. */
int
syd_thread_insert_timer_timeval (struct syd_thread_master *m, struct syd_thread *thread,
                             int (*func) (struct syd_thread *),
                             void *arg, struct timeval timer, char *name)
{
  if (syd_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  syd_thread_insert(m, thread, SYD_THREAD_TIMER, func, arg, name);

  syd_thread_update_timeval(thread, &timer);

  /* Common process.  */
  syd_thread_add_timer_common (m, thread);

  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Add simple event thread. */
struct syd_thread *
syd_thread_add_event (struct syd_thread_master *m,
                     int (*func) (struct syd_thread *),
                     void *arg, int val)
{
  struct syd_thread *thread;

  assert (m != NULL);

  thread = syd_thread_get (m, SYD_THREAD_EVENT, func, arg);
  if (thread == NULL)
    return NULL;

  syd_thread_add_thread_check(m, thread);

  thread->u.val = val;
  syd_thread_list_add (&m->event, thread);

  return thread;
}

/* Add new event thread with name. */
struct syd_thread *
syd_thread_add_event_withname (struct syd_thread_master * m,
                              int(* func)(struct syd_thread *),
                              void * arg, int val, char * name)
{
  struct syd_thread *thread;

  if (name == NULL)
    return syd_thread_add_event (m, func, arg, val);

  if (syd_thread_name_check (m, name))
    return NULL;

  thread = syd_thread_add_event (m, func, arg, val);
  if (thread)
    syd_thread_name_add (thread, name);

  return thread;
}

/* Add low priority event thread. */
struct syd_thread *
syd_thread_add_event_low (struct syd_thread_master *m,
                         int (*func) (struct syd_thread *),
                         void *arg, int val)
{
  struct syd_thread *thread;

  assert (m != NULL);

  thread = syd_thread_get (m, SYD_THREAD_EVENT_LOW, func, arg);
  if (thread == NULL)
    return NULL;

  syd_thread_add_thread_check(m, thread);

  thread->u.val = val;
  syd_thread_list_add (&m->event_low, thread);

  return thread;
}

/* Add new event low thread with name. */
struct syd_thread *
syd_thread_add_event_low_withname (struct syd_thread_master * m,
                                  int(* func)(struct syd_thread *),
                                  void * arg, int val, char * name)
{
  struct syd_thread *thread;

  if (name == NULL)
    return syd_thread_add_event_low (m, func, arg, val);

  if (syd_thread_name_check (m, name))
    return NULL;

  thread = syd_thread_add_event_low (m, func, arg, val);
  if (thread)
    syd_thread_name_add (thread, name);

  return thread;
}

/* Insert event thread. */
int
syd_thread_insert_event (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func) (struct syd_thread *),
        void *arg, int val, char *name)
{
  if (syd_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  syd_thread_insert(m, thread, SYD_THREAD_EVENT, func, arg, name);
  thread->u.val = val;
  syd_thread_list_add (&m->event, thread);

  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Insert event thread. */
int
syd_thread_insert_event_low (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func) (struct syd_thread *),
        void *arg, int val, char *name)
{
  if (syd_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  syd_thread_insert(m, thread, SYD_THREAD_EVENT_LOW, func, arg, name);
  thread->u.val = val;
  syd_thread_list_add (&m->event_low, thread);

  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Add pending read thread. */
struct syd_thread *
syd_thread_add_read_pend (struct syd_thread_master *m,
                         int (*func) (struct syd_thread *),
                         void *arg, int val)
{
  struct syd_thread *thread;

  assert (m != NULL);

  thread = syd_thread_get (m, SYD_THREAD_READ_PEND, func, arg);
  if (thread == NULL)
    return NULL;

  syd_thread_add_thread_check(m, thread);

  thread->u.val = val;
  syd_thread_list_add (&m->read_pend, thread);

  return thread;
}

/* Add new read pending thread with name. */
struct syd_thread *
syd_thread_add_read_pend_withname (struct syd_thread_master *m,
                                  int(* func)(struct syd_thread *),
                                  void * arg, int val, char * name)
{
  struct syd_thread *thread;

  if (name == NULL)
    return syd_thread_add_read_pend (m, func, arg, val);

  if (syd_thread_name_check (m, name))
    return NULL;

  thread = syd_thread_add_read_pend (m, func, arg, val);
  if (thread)
    syd_thread_name_add (thread, name);

  return thread;
}

/* Insert read pend thread. */
int
syd_thread_insert_read_pend (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func) (struct syd_thread *),
        void *arg, int val, char *name)
{
  if (syd_thread_insert_thread_arg_check(m, thread, name) != 0)
    return -1;

  syd_thread_insert(m, thread, SYD_THREAD_READ_PEND, func, arg, name);
  thread->u.val = val;
  syd_thread_list_add (&m->read_pend, thread);

  SYD_THREAD_SET_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR);

  return 0;
}

/* Cancel thread from scheduler. */
void
syd_thread_cancel (struct syd_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(thread != NULL);
#endif

  syd_thread_add_thread_check(thread->master, thread);

  switch (thread->type)
    {
    case SYD_THREAD_READ:
      if (SYD_THREAD_CHECK_FLAG(thread->master->flag, SYD_THREAD_USE_POLL))
        syd_thread_clear_pollfd (thread);
      else
        FD_CLR (thread->u.fd, &thread->master->readfd);
      syd_thread_list_delete (&thread->master->read, thread);
      break;
    case SYD_THREAD_READ_HIGH:
      if (SYD_THREAD_CHECK_FLAG(thread->master->flag, SYD_THREAD_USE_POLL))
        syd_thread_clear_pollfd (thread);
      else
        FD_CLR (thread->u.fd, &thread->master->readfd);
      syd_thread_list_delete (&thread->master->read_high, thread);
      break;
    case SYD_THREAD_WRITE:
      assert (FD_ISSET (thread->u.fd, &thread->master->writefd));
      if (SYD_THREAD_CHECK_FLAG(thread->master->flag, SYD_THREAD_USE_POLL))
        syd_thread_clear_pollfd (thread);
      else
        FD_CLR (thread->u.fd, &thread->master->writefd);
      syd_thread_list_delete (&thread->master->write, thread);
      break;
    case SYD_THREAD_TIMER:
      syd_thread_list_delete (&thread->master->timer[(int)thread->index], thread);
      break;
    case SYD_THREAD_EVENT:
      syd_thread_list_delete (&thread->master->event, thread);
      break;
    case SYD_THREAD_READ_PEND:
      syd_thread_list_delete (&thread->master->read_pend, thread);
      break;
    case SYD_THREAD_EVENT_LOW:
      syd_thread_list_delete (&thread->master->event_low, thread);
      break;
    case SYD_THREAD_QUEUE:
      switch (thread->priority)
        {
        case SYD_THREAD_PRIORITY_HIGH:
          syd_thread_list_delete (&thread->master->queue_high, thread);
          break;
        case SYD_THREAD_PRIORITY_MIDDLE:
          syd_thread_list_delete (&thread->master->queue_middle, thread);
          break;
        case SYD_THREAD_PRIORITY_LOW:
          syd_thread_list_delete (&thread->master->queue_low, thread);
          break;
        }
      break;
    default:
      SYD_THREAD_PROCEDURE_DBG (thread->master, "Invalid thread type %d.\n", thread->type);
      break;
    }
  syd_thread_unuse(thread->master, thread);
}

/* Delete all events which has argument value arg. */
void
syd_thread_cancel_event (struct syd_thread_master *m, void *arg)
{
  struct syd_thread *thread;
  struct syd_thread *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  thread = m->event.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          syd_thread_add_thread_check(t->master, thread);

          syd_thread_list_delete (&m->event, t);
          syd_thread_unuse(m, t);
        }
    }

  /* Since Event could have been Queued search queue_high */
  thread = m->queue_high.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          syd_thread_add_thread_check(t->master, thread);

          syd_thread_list_delete (&m->queue_high, t);
          syd_thread_unuse(m, t);
        }
    }

  return;
}

/* Delete all low-events which has argument value arg */
void
syd_thread_cancel_event_low (struct syd_thread_master *m, void *arg)
{
  struct syd_thread *thread;
  struct syd_thread *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  thread = m->event_low.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          syd_thread_add_thread_check(t->master, thread);

          syd_thread_list_delete (&m->event_low, t);
          syd_thread_unuse(m, t);
        }
    }

  /* Since Event could have been Queued search queue_low */
  thread = m->queue_low.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          syd_thread_add_thread_check(t->master, thread);

          syd_thread_list_delete (&m->queue_low, t);
          syd_thread_unuse(m, t);
        }
    }

  return;
}

/* Delete all read events which has argument value arg. */
void
syd_thread_cancel_read (struct syd_thread_master *m, void *arg)
{
  struct syd_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  thread = m->read.head;
  while (thread)
    {
      struct syd_thread *t;

      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          syd_thread_list_delete (&m->read, t);
          t->type = SYD_THREAD_UNUSED;
          syd_thread_add_unuse (m, t);
        }
    }
}

/* Delete all write events which has argument value arg. */
void
syd_thread_cancel_write (struct syd_thread_master *m, void *arg)
{
  struct syd_thread *thread;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  thread = m->write.head;
  while (thread)
    {
      struct syd_thread *t;

      t = thread;
      thread = t->next;

      if (t->arg == arg)
        {
          syd_thread_list_delete (&m->write, t);
          t->type = SYD_THREAD_UNUSED;
          syd_thread_add_unuse (m, t);
        }
    }
}

/* Delete all timer events which has ament value arg. */
void
syd_thread_cancel_timer (struct syd_thread_master *m, void *arg)
{
  struct syd_thread *thread;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
    {
      thread = m->timer[i].head;
      while (thread)
        {
          struct syd_thread *t;

          t = thread;
          thread = t->next;

          if (t->arg == arg)
            {
              syd_thread_list_delete (&m->timer[i], t);
              t->type = SYD_THREAD_UNUSED;
              syd_thread_add_unuse (m, t);
            }
        }
    }
}

#ifdef RTOS_DEFAULT_WAIT_TIME
struct timeval *
syd_thread_timer_wait (struct syd_thread_master *m, struct timeval *timer_val)
{
#ifdef LIB_PARAM_CHECK
  assert(timer_val != NULL);
#endif

  timer_val->tv_sec = 1;
  timer_val->tv_usec = 0;
  return timer_val;
}
#else /* ! RTOS_DEFAULT_WAIT_TIME */
#ifdef HAVE_RTOS_TIMER
struct timeval *
syd_thread_timer_wait (struct syd_thread_master *m, struct timeval *timer_val)
{
  rtos_set_time (timer_val);
  return timer_val;
}
#else /* ! HAVE_RTOS_TIMER */
#ifdef HAVE_RTOS_TIC
struct timeval *
syd_thread_timer_wait (struct syd_thread_master *m, struct timeval *timer_val)
{
#ifdef LIB_PARAM_CHECK
  assert(timer_val != NULL);
#endif

  timer_val->tv_sec = 0;
  timer_val->tv_usec = 10;
  return timer_val;
}
#else /* ! HAVE_RTOS_TIC */
#ifdef TIMER_NO_SORT
struct timeval *
syd_thread_timer_wait (struct syd_thread_master *m, struct timeval *timer_val)
{
  struct timeval timer_now;
  struct timeval timer_min;
  struct timeval *timer_wait;
  struct syd_thread *thread;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  timer_wait = NULL;

  for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
    for (thread = m->timer[i].head; thread; thread = thread->next)
      {
        if (! timer_wait)
          timer_wait = &thread->u.sands;
        else if (timeval_cmp (thread->u.sands, *timer_wait) < 0)
          timer_wait = &thread->u.sands;
      }

  if (timer_wait)
    {
      timer_min = *timer_wait;

      syd_time_tzcurrent (&timer_now, NULL);
      timer_min = timeval_subtract (timer_min, timer_now);

      if (timer_min.tv_sec < 0)
        {
          timer_min.tv_sec = 0;
          timer_min.tv_usec = 10;
        }

      *timer_val = timer_min;
      return timer_val;
    }
  return NULL;
}
#else /* ! TIMER_NO_SORT */
/* Pick up smallest timer.  */
struct timeval *
syd_thread_timer_wait (struct syd_thread_master *m, struct timeval *timer_val)
{
  struct timeval timer_now;
  struct timeval timer_min;
  struct timeval *timer_wait;
  struct syd_thread *thread;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
#endif

  timer_wait = NULL;

  for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
    if ((thread = m->timer[i].head) != NULL)
      {
        if (! timer_wait)
          timer_wait = &thread->u.sands;
        else if (timeval_cmp (thread->u.sands, *timer_wait) < 0)
          timer_wait = &thread->u.sands;
      }

  if (timer_wait)
    {
      timer_min = *timer_wait;

      syd_time_tzcurrent (&timer_now, NULL);
      timer_min = timeval_subtract (timer_min, timer_now);

      if (timer_min.tv_sec < 0)
        {
          timer_min.tv_sec = 0;
          timer_min.tv_usec = 10;
        }

      *timer_val = timer_min;
      return timer_val;
    }
  return NULL;
}
#endif /* TIMER_NO_SORT */
#endif /* HAVE_RTOS_TIC */
#endif /* HAVE_RTOS_TIMER */
#endif /* RTOS_DEFAULT_WAIT_TIME */

#ifdef HAVE_NGSA
/* Delete all events which has argument value arg and func ptr. */
void
syd_thread_cancel_event_by_arg_func (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg)
{
  struct syd_thread *thread;
  struct syd_thread *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(func != NULL);
#endif

  thread = m->event.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg && t->func == func)
        {
          syd_thread_add_thread_check(t->master, thread);

          syd_thread_list_delete (&m->event, t);
          syd_thread_unuse(m, t);
        }
    }

  /* Since Event could have been Queued search queue_high */
  thread = m->queue_high.head;
  while (thread)
    {
      t = thread;
      thread = t->next;

      if (t->arg == arg && t->func == func)
        {
          syd_thread_add_thread_check(t->master, thread);

          syd_thread_list_delete (&m->queue_high, t);
          syd_thread_unuse(m, t);
        }
    }

  return;
}

/* Delete thread which has argument value arg and func ptr, from the thread list. */
void
syd_thread_cancel_list_by_arg_func (struct syd_thread_master *m, struct syd_thread_list *list,
                                int (*func)(struct syd_thread *), void *arg)
{
  struct syd_thread *thread, *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
  assert(func != NULL);
  assert(arg != NULL);
#endif

  thread = list->head;
  while (thread)
    {
      t = thread;
      thread = t->next;
      if (t->arg == arg && t->func == func)
        {
          syd_thread_add_thread_check(t->master, thread);

          syd_thread_list_delete (list, t);
          syd_thread_unuse(m, t);
        }
    }
}

/* Delete all thread which has argument value arg and func ptr */
void
syd_thread_cancel_by_arg_func (struct syd_thread_master *m,
                           int (*func)(struct syd_thread *), void *arg)
{
  struct syd_thread_list *list;
  int i = 0;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(func != NULL);
  assert(arg != NULL);
#endif

  /* struct thread_list queue_high; */
  list = &m->queue_high;
  syd_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list queue_middle; */
  list = &m->queue_middle;
  syd_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list queue_low; */
  list = &m->queue_low;
  syd_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list timer[THREAD_TIMER_SLOT]; */
  for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
    {
      list = &m->timer[i];
      syd_thread_cancel_list_by_arg_func (m, list, func, arg);
    }

  /* struct thread_list read_pend; */
  list = &m->read_pend;
  syd_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list read_high; */
  list =&m->read_high;
  syd_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list read; */
  list = &m->read;
  syd_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list write; */
  list = &m->write;
  syd_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list event; */
  list = &m->event;
  syd_thread_cancel_list_by_arg_func (m, list, func, arg);

  /* struct thread_list event_low; */
  list = &m->event_low;
  syd_thread_cancel_list_by_arg_func (m, list, func, arg);
}

void
syd_thread_cancel_arg_check_list (struct syd_thread_master *m, struct syd_thread_list *list, void *arg)
{
  struct syd_thread *thread, *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
  assert(arg != NULL);
#endif

  thread = list->head;
  while (thread)
    {
      t = thread;
      thread = t->next;
      if (t->arg == arg)
        {
          assert(0);
#ifdef LIB_PARAM_CHECK
          SYD_THREAD_INTERFACE_DBG (m, "Function 0x%p in invalid thread list\n", t->func);
#endif
          syd_thread_add_thread_check(t->master, thread);

          syd_thread_list_delete (list, t);
          syd_thread_unuse(m, t);
        }
    }
}

void
syd_thread_cancel_arg_check (struct syd_thread_master *m, void *arg)
{
  struct syd_thread_list *list;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(arg != NULL);
#endif

  /* struct thread_list queue_high; */
  list = &m->queue_high;
  syd_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list queue_middle; */
  list = &m->queue_middle;
  syd_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list queue_low; */
  list = &m->queue_low;
  syd_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list timer[THREAD_TIMER_SLOT]; */
  for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
    {
      list = &m->timer[i];
      syd_thread_cancel_arg_check_list(m, list, arg);
    }

  /* struct thread_list read_pend; */
  list = &m->read_pend;
  syd_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list read_high; */
  list =&m->read_high;
  syd_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list read; */
  list = &m->read;
  syd_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list write; */
  list = &m->write;
  syd_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list event; */
  list = &m->event;
  syd_thread_cancel_arg_check_list(m, list, arg);

  /* struct thread_list event_low; */
  list = &m->event_low;
  syd_thread_cancel_arg_check_list(m, list, arg);
}

void
syd_thread_cancel_from_list_by_arg (struct syd_thread_master *m, struct syd_thread_list *list, void *arg)
{
  struct syd_thread *thread, *t;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(list != NULL);
  assert(arg != NULL);
#endif

  thread = list->head;
  while (thread)
    {
      t = thread;
      thread = t->next;
      if (t->arg== arg)
        {
          syd_thread_add_thread_check(t->master, thread);

          syd_thread_list_delete (list, t);
          syd_thread_unuse(m, t);
        }
    }
}

void
syd_thread_cancel_by_arg(struct syd_thread_master *m, void *arg)
{
  struct syd_thread_list *list;
  int i;

#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(arg != NULL);
#endif

  /* struct thread_list queue_high; */
  list = &m->queue_high;
  syd_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list queue_middle; */
  list = &m->queue_middle;
  syd_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list queue_low; */
  list = &m->queue_low;
  syd_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list timer[THREAD_TIMER_SLOT]; */
  for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
    {
      list = &m->timer[i];
      syd_thread_cancel_from_list_by_arg(m, list, arg);
    }

  /* struct thread_list read_pend; */
  list = &m->read_pend;
  syd_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list read_high; */
  list =&m->read_high;
  syd_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list read; */
  list = &m->read;
  syd_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list write; */
  list = &m->write;
  syd_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list event; */
  list = &m->event;
  syd_thread_cancel_from_list_by_arg(m, list, arg);

  /* struct thread_list event_low; */
  list = &m->event_low;
  syd_thread_cancel_from_list_by_arg(m, list, arg);
}

void
syd_thread_cancel_invalid_fd(struct syd_thread_master *m, struct syd_thread_list *list)
{
  struct syd_thread *thread;
  struct syd_thread *next;
  struct sockaddr address;
  unsigned int address_len;
  int ret;

#ifdef LIB_PARAM_CHECK
   assert(list != NULL);
#endif

  for (thread = list->head; thread; thread = next)
    {
      next = thread->next;
      ret = getsockname(SYD_THREAD_FD (thread), &address, &address_len);
      if (ret < 0)
          syd_thread_cancel(thread);
    }
}
#endif

struct syd_thread *
syd_thread_run (struct syd_thread_master *m, struct syd_thread *thread,
               struct syd_thread *fetch)
{
#ifdef LIB_PARAM_CHECK
  assert(thread != NULL);
#endif

  SYD_THREAD_PROCEDURE_DBG (m, "syd_thread run thread %p[%s], func %p.\n",
          thread, thread->name, thread->func);

  *fetch = *thread;

  syd_thread_unuse(m, thread);

  return fetch;
}

void
syd_thread_enqueue_high (struct syd_thread_master *m, struct syd_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(thread != NULL);
#endif

  thread->type = SYD_THREAD_QUEUE;
  thread->priority = SYD_THREAD_PRIORITY_HIGH;
  syd_thread_list_add (&m->queue_high, thread);
}

void
syd_thread_enqueue_middle (struct syd_thread_master *m, struct syd_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(thread != NULL);
#endif

  thread->type = SYD_THREAD_QUEUE;
  thread->priority = SYD_THREAD_PRIORITY_MIDDLE;
  syd_thread_list_add (&m->queue_middle, thread);
}

void
syd_thread_enqueue_low (struct syd_thread_master *m, struct syd_thread *thread)
{
#ifdef LIB_PARAM_CHECK
  assert(m != NULL);
  assert(thread != NULL);
#endif

  thread->type = SYD_THREAD_QUEUE;
  thread->priority = SYD_THREAD_PRIORITY_LOW;
  syd_thread_list_add (&m->queue_low, thread);
}

/* When the file is ready move to queueu.  */
int
syd_thread_process_fd (struct syd_thread_master *m, struct syd_thread_list *list,
                      fd_set *fdset, fd_set *mfdset)
{
  struct syd_thread *thread;
  struct syd_thread *next;
  int ready = 0;

#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  for (thread = list->head; thread; thread = next)
    {
      next = thread->next;

      if (FD_ISSET (SYD_THREAD_FD (thread), fdset))
        {
          FD_CLR(SYD_THREAD_FD (thread), mfdset);
          syd_thread_list_delete (list, thread);
          syd_thread_enqueue_middle (m, thread);
          ready++;
        }
    }
  return ready;
}

/* When the file is ready move to queueu.  */
int
syd_thread_process_poll_fd (struct syd_thread_master *m, struct syd_thread_list *list,
                      int found, bool is_write)
{
  struct syd_thread *thread;
  struct syd_thread *next;
  short revents;
  int ready = 0;

#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  for (thread = list->head; thread && found > 0; thread = next)
    {
      next = thread->next;

      if (m->pfds[thread->pfd_index].fd < 0)
        {
          /* 该伪线程对应的pollfd没有设置，说明还没有被侦听 */
          SYD_THREAD_PROCEDURE_DBG(m,
              "Thread not listened, %s, %d\n", thread->name, thread->pfd_index);
          continue;
        }

      revents = m->pfds[thread->pfd_index].revents;

      if (revents == 0)
        {
          /* 没有事件发生 */
          SYD_THREAD_PROCEDURE_DBG(m,
              "Nothing happend, %s, %d\n", thread->name, thread->pfd_index);
          continue;
        }

      if (is_write)
        {
          if (!(revents & (POLLOUT | POLLWRNORM | POLLWRBAND)))
            {
              /* 如果是遍历写伪线程，却没有返回可写事件，就不用继续了 */
              SYD_THREAD_PROCEDURE_DBG(m,
                  "Write thread recved pollin, %s, %d, %d\n",
                  thread->name, thread->pfd_index, revents);
              continue;
            }
        }
      else
        {
          /*
           * 增加POLLHUP是因为poll时，如果直接创建socket没有connect此时去侦听会返回这个值；
           * 此外，连接断开时会返回POLLHUP | POLLIN；
           */
          if (!(revents & (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLHUP)))
            {
              /* 如果是遍历读伪线程，却没有返回可读事件，就不用继续了 */
              SYD_THREAD_PROCEDURE_DBG(m,
                  "Read thread recved pollout, %s, %d, %d\n",
                  thread->name, thread->pfd_index, revents);
              continue;
            }
        }

      found--;
      /* 将伪线程对应的pollfd数组清空，避免再次被侦听 */
      syd_thread_clear_pollfd (thread);

      /* 将伪线程放入可执行队列中 */
      syd_thread_list_delete (list, thread);
      syd_thread_enqueue_middle (m, thread);
      if (SYD_THREAD_DEBUG_PROCEDURE_ON (m))
        syd_thread_disp_thread(thread);
      ready++;
    }

  return ready;
}

/* syd-thread type strings. */
static char *syd_thread_get_type_str (char type, char pri)
{
  switch (type)
    {
    case SYD_THREAD_READ:
      return "read";
    case SYD_THREAD_WRITE:
      return "write";
    case SYD_THREAD_TIMER:
      return "timer";
    case SYD_THREAD_EVENT:
      return "event";
    case SYD_THREAD_QUEUE:
      switch (pri)
        {
        case SYD_THREAD_PRIORITY_HIGH:
          return "high";
        case SYD_THREAD_PRIORITY_MIDDLE:
          return "middle";
        case SYD_THREAD_PRIORITY_LOW:
          return "low";
        default:
          assert (0);
          return NULL;
        }
      break;
    case SYD_THREAD_UNUSED:
      return "unuse";
    case SYD_THREAD_READ_HIGH:
      return "read high";
    case SYD_THREAD_READ_PEND:
      return "read pend";
    case SYD_THREAD_EVENT_LOW:
      return "event low";
    default:
      assert (0);
      return NULL;
    }

  return NULL;
}

static void
syd_thread_disp_invalid_fd_thread (struct syd_thread *thread)
{
  if (thread == NULL)
    return;

  SYD_THREAD_PRINT ("syd_thread display thread:\n");
  SYD_THREAD_PRINT (" prev:%p\n", thread->prev);
  SYD_THREAD_PRINT (" next:%p\n", thread->next);
  SYD_THREAD_PRINT (" master:%p\n", thread->master);
  SYD_THREAD_PRINT (" func:%p\n", thread->func);
  SYD_THREAD_PRINT (" zg:%p\n", thread->zg);
  SYD_THREAD_PRINT (" arg:%p\n", thread->arg);
  SYD_THREAD_PRINT (" type:%s(%d,%d)\n", syd_thread_get_type_str (thread->type, thread->priority),
                   thread->type, thread->priority);
  SYD_THREAD_PRINT (" index:%d\n", thread->index);
  SYD_THREAD_PRINT (" fd:%d\n", thread->u.fd);
  SYD_THREAD_PRINT (" name:%s\n", thread->name);

  return;
}

static void
syd_thread_disp_invalid_fd_list (struct syd_thread_master *m, struct syd_thread_list *list)
{
  struct syd_thread *thread;
  struct syd_thread *next;
  struct sockaddr address;
  unsigned int address_len;
  int ret;

#ifdef LIB_PARAM_CHECK
  assert(list != NULL);
#endif

  for (thread = list->head; thread; thread = next)
    {
      next = thread->next;
      ret = getsockname(SYD_THREAD_FD (thread), &address, &address_len);
      if (ret < 0)
        syd_thread_disp_invalid_fd_thread (thread);

      if (SYD_THREAD_FD (thread) > 1024)//wex
        {
          SYD_THREAD_PRINT("unsupport fd %d\n", SYD_THREAD_FD (thread));
          syd_thread_disp_invalid_fd_thread (thread);
        }
    }

  return;
}

static void
syd_thread_disp_invalid_fd (struct syd_thread_master *m)
{
  struct syd_thread_list *list;

#ifdef LIB_PARAM_CHECK
  assert (m != NULL);
#endif

  /* struct thread_list read_high; */
  list =&m->read_high;
  syd_thread_disp_invalid_fd_list(m, list);

  /* struct thread_list read; */
  list = &m->read;
  syd_thread_disp_invalid_fd_list(m, list);

  /* struct thread_list write; */
  list = &m->write;
  syd_thread_disp_invalid_fd_list (m, list);

  return;
}

/* Fetch next ready thread. */
struct syd_thread *
syd_thread_fetch (struct syd_thread_master *m, struct syd_thread *fetch)
{
  int num;
  struct syd_thread *thread;
  struct syd_thread *next;
  fd_set readfd;
  fd_set writefd;
  fd_set exceptfd;
  struct timeval timer_now;
  struct timeval timer_val;
  struct timeval *timer_wait;
  struct timeval timer_nowait;
  long timeout_poll;  /* poll机制使用的超时时间 */
  int i;

  if (m == NULL)
    {
      assert(0);
      return NULL;
    }
  /*获取运行线程ID*/
  if (SYD_THREAD_CHECK_FLAG(m->flag, SYD_THREAD_IS_FIRST_FETCH))
    {
      m->tid = syd_thread_gettid();
      SYD_THREAD_UNSET_FLAG(m->flag, SYD_THREAD_IS_FIRST_FETCH);
    }

#ifdef RTOS_DEFAULT_WAIT_TIME
  /* 1 sec might not be optimized */
  timer_nowait.tv_sec = 1;
  timer_nowait.tv_usec = 0;
#else
  timer_nowait.tv_sec = 0;
  timer_nowait.tv_usec = 0;
#endif /* RTOS_DEFAULT_WAIT_TIME */

  while (1)
    {
      /* Pending read is exception. */
      if ((thread = syd_thread_trim_head (&m->read_pend)) != NULL)
        return syd_thread_run (m, thread, fetch);

      /* Check ready queue.  */
      if ((thread = syd_thread_trim_head (&m->queue_high)) != NULL)
        return syd_thread_run (m, thread, fetch);

      if ((thread = syd_thread_trim_head (&m->queue_middle)) != NULL)
        return syd_thread_run (m, thread, fetch);

      if ((thread = syd_thread_trim_head (&m->queue_low)) != NULL)
        return syd_thread_run (m, thread, fetch);

      /* Check all of available events.  */

      /* Check events.  */
      while ((thread = syd_thread_trim_head (&m->event)) != NULL)
        syd_thread_enqueue_high (m, thread);

      /* Check timer.  */
      syd_time_tzcurrent (&timer_now, NULL);

      for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
        for (thread = m->timer[i].head; thread; thread = next)
          {
            next = thread->next;
            if (timeval_cmp (timer_now, thread->u.sands) >= 0)
              {
                syd_thread_list_delete (&m->timer[i], thread);
                syd_thread_enqueue_middle (m, thread);
              }
#ifndef TIMER_NO_SORT
            else
              break;
#endif /* TIMER_NO_SORT */
          }

      /* Low priority events. */
      if ((thread = syd_thread_trim_head (&m->event_low)) != NULL)
        syd_thread_enqueue_low (m, thread);

      /* Structure copy.  */
      readfd = m->readfd;
      writefd = m->writefd;
      exceptfd = m->exceptfd;

      /* Check any thing to be execute.  */
      if (m->queue_high.head || m->queue_middle.head || m->queue_low.head)
        timer_wait = &timer_nowait;
      else
        timer_wait = syd_thread_timer_wait (m, &timer_val);

      /* First check for sockets.  Return immediately.  */
      if (SYD_THREAD_CHECK_FLAG(m->flag, SYD_THREAD_USE_POLL))
        {
          /* poll机制使用的超时时间为毫秒 */
          if (timer_wait != NULL)
            timeout_poll = timer_wait->tv_sec * 1000 + timer_wait->tv_usec / 1000;
          else
            timeout_poll = -1;

          num = poll (m->pfds, m->nfds + 1, timeout_poll);
        }
      else
        {
          num = select (m->max_fd + 1, &readfd, &writefd, &exceptfd,
                                 timer_wait);
        }

      /* Error handling.  */
      if (num < 0)
        {
          /* 中断信号打断，只需要继续就行 */
          if (errno == EINTR)
            {
              if (SYD_THREAD_DEBUG_PROCEDURE_ON (m))
                syd_thread_disp_invalid_fd (m);
              continue;
            }

          /* 没内存，继续尝试即可 */
          if (errno == ENOMEM)
            {
              SYD_THREAD_PROCEDURE_DBG (m, "No memory to select, just try again.\n");
              sleep (2 * HZ);
              continue;
            }

          /* 出现了某种错误，此时强制打印信息吧，反正已经有BUG了 */
          SYD_THREAD_PRINT ("Select error: %d pid:%lu\n", errno, getpid());
          syd_thread_disp_invalid_fd (m);
          assert(0);
          return NULL;
        }

      /* File descriptor is readable/writable.  */
      if (num > 0)
        {
          /* High priority read thead. */
          if (SYD_THREAD_CHECK_FLAG(m->flag, SYD_THREAD_USE_POLL))
            syd_thread_process_poll_fd (m, &m->read_high, num, false);
          else
            syd_thread_process_fd (m, &m->read_high, &readfd, &m->readfd);

          /* Normal priority read thead. */
          if (SYD_THREAD_CHECK_FLAG(m->flag, SYD_THREAD_USE_POLL))
            syd_thread_process_poll_fd (m, &m->read, num, false);
          else
            syd_thread_process_fd (m, &m->read, &readfd, &m->readfd);

          /* Write thead. */
          if (SYD_THREAD_CHECK_FLAG(m->flag, SYD_THREAD_USE_POLL))
            syd_thread_process_poll_fd (m, &m->write, num, true);
          else
            syd_thread_process_fd (m, &m->write, &writefd, &m->writefd);
        }
    }

  return NULL;  /* Never reach here */
}

/*2013-1-11 linyuhui 添加显示耗时大于9秒的函数*/

struct timeval ts1;
struct timeval te1;
struct timeval tsub1;

/* Call the thread.  */
void
syd_thread_call (struct syd_thread *thread)
{
  syd_time_tzcurrent (&ts1, NULL);
  (*thread->func) (thread);
  syd_time_tzcurrent (&te1, NULL);
  tsub1 = timeval_subtract(te1, ts1);
  if (tsub1.tv_sec >= 9)  	
    {
      SYD_THREAD_PRINT ("thread used %d.%06d sec", tsub1.tv_sec, tsub1.tv_usec);
      syd_thread_disp_thread(thread);
    }
}

/* Fake execution of the thread with given arguemment.  */
struct syd_thread *
syd_thread_execute (int (*func)(struct syd_thread *),
                void *arg,
                int val)
{
  struct syd_thread dummy;

  memset (&dummy, 0, sizeof (struct syd_thread));

  dummy.type = SYD_THREAD_EVENT;
  dummy.master = NULL;
  dummy.func = func;
  dummy.arg = arg;
  dummy.u.val = val;
  syd_thread_call (&dummy);

  return NULL;
}

/* Real time OS support routine.  */
#ifdef HAVE_RTOS_TIC
#ifdef RTOS_EXECUTE_ONE_THREAD
int
syd_lib_tic (struct syd_thread_master *master, struct syd_thread *thread)
{
  if (syd_thread_fetch (master, thread))
    {
      syd_thread_call(thread);
      /* To indicate that a thread has run.  */
      return(1);
    }
  /* To indicate that no thread has run yet.  */
  return (0);
}
#else /* ! RTOS_EXECUTE_ONE_THREAD */
int
syd_syd_lib_tic (struct syd_thread_master *master, struct syd_thread *thread)
{
  while (syd_thread_fetch (master, thread))
    syd_thread_call(thread);
  return(0);
}
#endif /* ! RTOS_EXECUTE_ONE_THREAD */
#endif /* HAVE_RTOS_TIC */

/* Display thread master */
void
syd_thread_disp_thread_master (struct syd_thread_master *m)
{
  int i;

  SYD_THREAD_PRINT ("syd_thread Master Display:\n");
  SYD_THREAD_PRINT (" Queue-high: head %p tail %p count %u\n",
                   m->queue_high.head, m->queue_high.tail, m->queue_high.count);
  SYD_THREAD_PRINT (" Queue-middle: head %p tail %p count %u\n",
                   m->queue_middle.head, m->queue_middle.tail, m->queue_middle.count);
  SYD_THREAD_PRINT (" Queue-low: head %p tail %p count %u\n",
                   m->queue_low.head, m->queue_low.tail, m->queue_low.count);
  for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
    SYD_THREAD_PRINT (" Queue-timer[%d]: head %p tail %p count %u\n",
                     i, m->timer[i].head, m->timer[i].tail, m->timer[i].count);
  SYD_THREAD_PRINT (" Queue-read-pend: head %p tail %p count %u\n",
                   m->read_pend.head, m->read_pend.tail, m->read_pend.count);
  SYD_THREAD_PRINT (" Queue-read-high: head %p tail %p count %u\n",
                   m->read_high.head, m->read_high.tail, m->read_high.count);
  SYD_THREAD_PRINT (" Queue-read: head %p tail %p count %u\n",
                   m->read.head, m->read.tail, m->read.count);
  SYD_THREAD_PRINT (" Queue-write: head %p tail %p count %u\n",
                   m->write.head, m->write.tail, m->write.count);
  SYD_THREAD_PRINT (" Queue-event: head %p tail %p count %u\n",
                   m->event.head, m->event.tail, m->event.count);
  SYD_THREAD_PRINT (" Queue-event-low: head %p tail %p count %u\n",
                   m->event_low.head, m->event_low.tail, m->event_low.count);

  SYD_THREAD_PRINT (" Max_fd: %d\n", m->max_fd);

  SYD_THREAD_PRINT (" Poll nfds:%d\n", m->nfds);
  SYD_THREAD_PRINT (" Poll pfds_size:%d\n", m->pfds_size);

  SYD_THREAD_PRINT (" Unuse syd-thread count:%u\n", m->unuse.count);
  SYD_THREAD_PRINT (" Total syd-thread count:%u\n", m->alloc);

  return;
}

/* Display thread queue */
static void
syd_thread_disp_thread_queue_detail (struct syd_thread_list *list)
{
  struct syd_thread *thread;
  int i;

  SYD_THREAD_PRINT (" count:%u\n", list->count);
  if (list->count)
    {
      SYD_THREAD_PRINT (" ");
    }
  else
    {
      return;
    }

  thread = list->head;
  i = 0;
  while (thread)
    {
      SYD_THREAD_PRINT ("Thread:%p", thread);
      if (strlen (thread->name))
        {
          SYD_THREAD_PRINT ("name: [%s]", thread->name);
        }
      else
        {
          SYD_THREAD_PRINT ("name: [NULL]");
        }

      thread = thread->next;
      if (thread)
        {
          SYD_THREAD_PRINT ("-->");

          i++;
          if (i % 5 == 0)
            {
              SYD_THREAD_PRINT ("\n");
              i = 0;
            }
        }
      else
        {
          SYD_THREAD_PRINT ("\n");
          return;
        }
    }

  return;
}

void
syd_thread_disp_thread_queue (struct syd_thread_master *m, int queue_type)
{
  assert (m);

  if (queue_type < SYD_THREAD_READ || queue_type > SYD_THREAD_EVENT_LOW)
    return;

  switch (queue_type)
    {
    case SYD_THREAD_READ:
      SYD_THREAD_PRINT ("syd_thread display read-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->read);
      break;
    case SYD_THREAD_WRITE:
      SYD_THREAD_PRINT ("syd_thread display write-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->write);
      break;
    case SYD_THREAD_TIMER:
      {
        int i;
        for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
          {
            SYD_THREAD_PRINT ("syd_thread display time-%d-queue:\n", i);
            syd_thread_disp_thread_queue_detail (&m->timer[i]);
          }
        break;
      }
    case SYD_THREAD_EVENT:
      SYD_THREAD_PRINT ("syd_thread display event-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->event);
      break;
    case SYD_THREAD_QUEUE:
      SYD_THREAD_PRINT ("syd_thread display high-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->queue_high);
      SYD_THREAD_PRINT ("syd_thread display middle-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->queue_middle);
      SYD_THREAD_PRINT ("syd_thread display low-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->queue_low);
      break;
    case SYD_THREAD_UNUSED:
      SYD_THREAD_PRINT ("syd_thread display unuse-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->unuse);
      break;
    case SYD_THREAD_READ_HIGH:
      SYD_THREAD_PRINT ("syd_thread display read-high-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->read_high);
      break;
    case SYD_THREAD_READ_PEND:
      SYD_THREAD_PRINT ("syd_thread display read-pend-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->read_pend);
      break;
    case SYD_THREAD_EVENT_LOW:
      SYD_THREAD_PRINT ("syd_thread display event-low-queue:\n");
      syd_thread_disp_thread_queue_detail (&m->event_low);
      break;
    default:
      break;
    }

  return;
}

void
syd_thread_disp_thread (struct syd_thread *thread)
{
  if (thread == NULL)
    return;

  SYD_THREAD_PRINT ("syd_thread display thread:\n");
  SYD_THREAD_PRINT (" prev:%p\n", thread->prev);
  SYD_THREAD_PRINT (" next:%p\n", thread->next);
  SYD_THREAD_PRINT (" master:%p\n", thread->master);
  SYD_THREAD_PRINT (" func:%p\n", thread->func);
  SYD_THREAD_PRINT (" zg:%p\n", thread->zg);
  SYD_THREAD_PRINT (" arg:%p\n", thread->arg);
  SYD_THREAD_PRINT (" type:%s(%d,%d)\n", syd_thread_get_type_str (thread->type, thread->priority),
                   thread->type, thread->priority);
  SYD_THREAD_PRINT (" index:%d\n", thread->index);
  SYD_THREAD_PRINT (" val/fd/timer:%d\n", thread->u.val);
  SYD_THREAD_PRINT (" name:%s\n", thread->name);
  SYD_THREAD_PRINT (" creater:%s\n",
          SYD_THREAD_CHECK_FLAG(thread->flag, SYD_THREAD_FLAG_CRT_BY_USR) ? "USER" : "SYD-THREAD");

  return;
}

static void
syd_thread_disp_thread_byname_check_list (struct syd_thread_list *list, char *name)
{
  struct syd_thread *thread;

  thread = list->head;
  while (thread)
    {
      if (strcmp (thread->name, name) == 0)
        syd_thread_disp_thread (thread);
      thread = thread->next;
    }

  return;
}

void
syd_thread_disp_thread_byname (struct syd_thread_master *m, char *name)
{
  int i;

  if (name == NULL)
    return;

  syd_thread_disp_thread_byname_check_list (&m->queue_high, name);
  syd_thread_disp_thread_byname_check_list (&m->queue_middle, name);
  syd_thread_disp_thread_byname_check_list (&m->queue_low, name);

  for (i = 0; i < SYD_THREAD_TIMER_SLOT; i++)
    {
      syd_thread_disp_thread_byname_check_list (&m->timer[i], name);
    }

  syd_thread_disp_thread_byname_check_list (&m->read_pend, name);
  syd_thread_disp_thread_byname_check_list (&m->read_high, name);
  syd_thread_disp_thread_byname_check_list (&m->read, name);
  syd_thread_disp_thread_byname_check_list (&m->write, name);
  syd_thread_disp_thread_byname_check_list (&m->event, name);
  syd_thread_disp_thread_byname_check_list (&m->event_low, name);
  syd_thread_disp_thread_byname_check_list (&m->unuse, name);

  return;
}

void
syd_thread_debug_interface (struct syd_thread_master *m, int debug_switch)
{
  if (m == NULL)
    return;

  if (debug_switch == SYD_THREAD_DEBUG_OFF)
    {
      SYD_THREAD_UNSET_FLAG (m->debug, SYD_THREAD_DEBUG_INTERFACE_FLAG);
    }
  else if (debug_switch == SYD_THREAD_DEBUG_ON)
    {
      SYD_THREAD_SET_FLAG (m->debug, SYD_THREAD_DEBUG_INTERFACE_FLAG);
    }
  else
    {
      SYD_THREAD_PRINT ("Init syd_thread debug interface param[%d] failed:\n", debug_switch);
    }

  return;
}

void
syd_thread_debug_procedure (struct syd_thread_master *m, int debug_switch)
{
  if (m == NULL)
    return;

  if (debug_switch == SYD_THREAD_DEBUG_OFF)
    {
      SYD_THREAD_UNSET_FLAG (m->debug, SYD_THREAD_DEBUG_PROCEDURE_FLAG);
    }
  else if (debug_switch == SYD_THREAD_DEBUG_ON)
    {
      SYD_THREAD_SET_FLAG (m->debug, SYD_THREAD_DEBUG_PROCEDURE_FLAG);
    }
  else
    {
      SYD_THREAD_PRINT ("Init syd_thread debug procedure param[%d] failed:\n", debug_switch);
    }

  return;
}

struct syd_thread *
syd_thread_add_recover_timer (struct syd_thread_master *m,
                             int (*func) (struct syd_thread *),
                             void *arg, long timer)
{
  struct syd_thread *thread;

  if (m == NULL)
    return NULL;

  if (m->t_recover)
    {
      /* 恢复线程处于未使用状态，重新入等待队列 */
      if (m->t_recover->type == SYD_THREAD_UNUSED)
        {
          struct timeval timer_now;

          thread = m->t_recover;

          syd_time_tzcurrent (&timer_now, NULL);
#ifdef HAVE_NGSA
          if (timer < 0)
            {
              timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
            }
          else
            {
              timer_now.tv_sec += timer;
              if (timer_now.tv_sec < 0)
                timer_now.tv_sec = PAL_TIME_MAX_TV_SEC;
            }
#else
          timer_now.tv_sec += timer;
#endif
          /* 更新定时器参数 */
          thread->u.sands = timer_now;

          /* 添加定时器线程 */
          syd_thread_add_thread_check(m, thread);
          syd_thread_add_timer_common (m, thread);

          thread->type = SYD_THREAD_TIMER;
          thread->func = func;
          thread->arg = arg;

          return thread;
        }
      else
        {
          /* 伪线程处于已使用状态，添加失败(只允许添加一个恢复伪线程) */
          SYD_THREAD_INTERFACE_DBG (m, "Could not modify recover timer while it's running.\n");
          return NULL;
        }
    }
  else
    {
      thread = syd_thread_add_timer (m, func, arg, timer);
      if (thread == NULL)
        {
          SYD_THREAD_INTERFACE_DBG (m, "Failed to add recover timer.\n");
        }
      else
        {
          syd_thread_add_thread_check(m, thread);
          m->t_recover = thread;
        }

      return thread;
    }
}

struct syd_thread *
syd_thread_add_recover_timer_withname (struct syd_thread_master *m,
                                      int (*func) (struct syd_thread *),
                                      void *arg, long timer, char *name)
{
  struct syd_thread *thread;

  if (name == NULL)
    return syd_thread_add_recover_timer (m, func, arg, timer);

  if (syd_thread_name_check (m, name))
    return NULL;

  thread = syd_thread_add_recover_timer (m, func, arg, timer);
  if (thread)
    syd_thread_name_add (thread, name);

  return thread;
}

void
syd_thread_cancel_recover_timer (struct syd_thread_master *m)
{
#ifdef LIB_PARAM_CHECK
  assert (m != NULL);
#endif

  if (m->t_recover == NULL)
    return;

  syd_thread_cancel (m->t_recover);
  m->t_recover = NULL;

  return;
}
