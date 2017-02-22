/* Copyright (C) 2001-2003 IP Infusion, Inc. All Rights Reserved. */

#ifndef _SYD_THREAD_H
#define _SYD_THREAD_H

//#include <syd_lib/syd_syslog.h>
#include <stdio.h>
#include "lib.h"

/**
 * @defgroup    SERVICE_SYD_THREAD     Native API
 *
 * 伪线程API接口。头文件<libpub/syd_thread/syd_thread.h>
 *
 * <p>
 * 伪线程并非实际意义上的操作系统线程，它不存在于操作系统的调度队列中。
 * 对于使用伪线程机制的模块来说，一般情况下该模块会创建一个真实的线程，
 * 然后在这个线程中进行伪线程的调度。<br>
 * 由于伪线程机制中的线程并非真正意义上的操作系统线程，因此某个伪线程挂住将使得伪线程调度器
 * 无法再继续运行，也就会造成相应的操作系统进程或线程挂住。因此，如果要求操作系统进程或线程不挂住，
 * 那么伪线程的处理中不能有挂起操作。在设计考虑的时候，通过伪线程调度机制一般情况下可以做到
 * 伪线程的处理不挂起。例如，对于socket读写操作，不再使用原先的直接尝试去阻塞读写的方式，
 * 而是添加一个读或写伪线程，伪线程调度机制在判断socket可读或者可写的时候才进行读或写操作。
 * <p>
 * 正如实际操作系统线程一样，伪线程也同样具有优先级这一特性，线程优先级主要是可以让使用者有更多
 * 的选择来安排各类事件的调度顺序。伪线程机制具有四种优先级:
 * 最高优先级、高优先级、普通优先级、低优先级。
 * <p>
 * 伪线程机制是一个无限循环的机制，也就是说使用伪线程的模块将一直循环查找可以运行的伪线程，
 * 找到可运行的线程后将其调度执行。因此，在系统初始化的时候，至少需要添加一个伪线程，否则后续
 * 进入无限循环后由于没有可运行的线程，将一直无法添加新的伪线程进去。<br>
 * <b>注意:伪线程机制只允许一个操作系统线程对其进行操作，不能够有两个以上的操作系统线程同时对一个
 * 伪线程管理器中的伪线程进行添加、删除等。</b>
 * <p>
 * 下面是个最简单的使用伪线程的例子：
 *
 * @code
 * #include <libpub/syd_thread/syd_thread.h>
 *
 * #define TEST_TIME 5
 *
 * struct syd_thread_master *g_master;
 * struct syd_thread g_timer;
 *
 * int test_timer(struct syd_thread *t)
 * {
 *     printf("test timer exipre\n");
 *
 *     (void)syd_thread_insert_timer(g_master, &g_timer, test_timer, NULL, TEST_TIME, "g-timer");
 *
 *     return 0;
 * }
 *
 * int main(void)
 * {
 *     struct syd_thread thread;
 *
 *     g_master = syd_thread_master_create();
 *     if (g_master == NULL) {
 *         return -1;
 *     }
 *
 *     (void)syd_thread_insert_timer(g_master, &g_timer, test_timer, NULL, TEST_TIME, "g-timer");
 *
 *     while (syd_thread_fetch (g_master, &thread)) {
 *         syd_thread_call (&thread);
 *     }
 *
 *     return 0;
 * }
 * @endcode
 *
 * @ingroup     SERVICE_SYD_THREAD
 *
 * @{
 */
/*add  by  syd2679*/ 
typedef unsigned int uint32_t; 
#define bool int
enum {
	false	= 0,
	true	= 1
};

/*end for  sunnada add*/


/* 无需抽取注释的定义 */
#define HAVE_NGSA

/* Flag manipulation macros. */
#define SYD_THREAD_CHECK_FLAG(V,F)       ((V) & (F))
#define SYD_THREAD_SET_FLAG(V,F)         (V) = (V) | (F)
#define SYD_THREAD_UNSET_FLAG(V,F)       (V) = (V) & ~(F)

#define SYD_THREAD_DEBUG_INTERFACE_ON(m) SYD_THREAD_CHECK_FLAG(m->debug, SYD_THREAD_DEBUG_INTERFACE_FLAG)
#define SYD_THREAD_DEBUG_PROCEDURE_ON(m) SYD_THREAD_CHECK_FLAG(m->debug, SYD_THREAD_DEBUG_PROCEDURE_FLAG)

#if 1
#define SYD_THREAD_PRINT printf
#else
#define SYD_THREAD_PRINT (void)0;(void)
#endif

/* Debug interface */
#define SYD_THREAD_INTERFACE_DBG(m, fmt, ...)                          \
  do {                                                                        \
    if (SYD_THREAD_DEBUG_INTERFACE_ON (m))                                     \
      {                                                                       \
        SYD_THREAD_PRINT ("[THREAD]" fmt, ## __VA_ARGS__);                                \
      }                                                                       \
  } while (0)

/* Debug procedure */
#define SYD_THREAD_PROCEDURE_DBG(m, fmt, ...)                          \
  do {                                                                        \
    if (SYD_THREAD_DEBUG_PROCEDURE_ON (m))                                     \
      {                                                                       \
        SYD_THREAD_PRINT ("[THREAD]" fmt, ## __VA_ARGS__);                                \
      }                                                                       \
  } while (0)


/* Thread types.  */
#define SYD_THREAD_READ              0
#define SYD_THREAD_WRITE             1
#define SYD_THREAD_TIMER             2
#define SYD_THREAD_EVENT             3
#define SYD_THREAD_QUEUE             4
#define SYD_THREAD_UNUSED            5
#define SYD_THREAD_READ_HIGH         6
#define SYD_THREAD_READ_PEND         7
#define SYD_THREAD_EVENT_LOW         8

#define SYD_THREAD_UNUSE_MAX         1024
#define SYD_THREAD_FREE_UNUSE_MAX    (SYD_THREAD_UNUSE_MAX >> 1)

/* Linked list of thread. */
struct syd_thread_list
{
  struct syd_thread *head;
  struct syd_thread *tail;
  u_int32_t count;
};

#define SYD_THREAD_DEBUG_OFF         0
#define SYD_THREAD_DEBUG_ON          1
 /* 以上为无需抽取注释的定义 */

/**
 * 伪线程管理器.
 * 在使用伪线程机制时，必须创建一个伪线程管理器，用于管理伪线程。
 */
struct syd_thread_master
{
  /* Priority based queue.  */
  struct syd_thread_list queue_high;
  struct syd_thread_list queue_middle;
  struct syd_thread_list queue_low;

  /* Timer */
#define SYD_THREAD_TIMER_SLOT           4
  int index;
  struct syd_thread_list timer[SYD_THREAD_TIMER_SLOT];

  /* Thread to be executed.  */
  struct syd_thread_list read_pend;
  struct syd_thread_list read_high;
  struct syd_thread_list read;
  struct syd_thread_list write;
  struct syd_thread_list event;
  struct syd_thread_list event_low;
  struct syd_thread_list unuse;
  fd_set readfd;
  fd_set writefd;
  fd_set exceptfd;
  int max_fd;

  /* 使用poll机制 */
  struct pollfd *pfds;      /**< pollfd数组 */
  short nfds;               /**< pollfd有效数组大小，伪线程库内部使用，用户不要自己设置 */
  short pfds_size;          /**< pollfd数组大小，伪线程库内部使用，用户不要自己设置 */

  u_int32_t alloc;
  int debug;
#define SYD_THREAD_DEBUG_INTERFACE_FLAG     (1 << 0)
#define SYD_THREAD_DEBUG_PROCEDURE_FLAG     (1 << 1)

  u_int32_t flag;
#define SYD_THREAD_IS_FIRST_FETCH (1 << 0)
#define SYD_THREAD_USE_POLL (1 << 1)

  int tid;   /*线程ID*/

  struct syd_thread *t_recover;
};

#define SYD_THREAD_NAME_LEN          10  /* 伪线程名最大长度 */

/**
 * 伪线程。
 */
struct syd_thread
{
  struct syd_thread *next;
  struct syd_thread *prev;

  struct syd_thread_master *master;

  int (*func) (struct syd_thread *);   /**< 伪线程执行函数 */

  void *zg;                           /**< zebos模块使用的lib_globals */

  void *arg;                          /**< 事件参数，一般传递结构体指针 */

  char type;                          /**< 伪线程类型，伪线程库使用，用户不要自己设置 */

  char priority;                      /**< 伪线程优先级，不自己设置，通过加入不同类型伪线程时由伪线程库自动设置 */
#define SYD_THREAD_PRIORITY_HIGH         0
#define SYD_THREAD_PRIORITY_MIDDLE       1
#define SYD_THREAD_PRIORITY_LOW          2

  char index;                         /**< 伪线程定时器数组索引，伪线程库使用，用户不要自己设置 */

  char flag;                          /**< 伪线程标志，伪线程库使用，用户不要自己设置 */
#define SYD_THREAD_FLAG_CRT_BY_USR       (1 << 0)
#define SYD_THREAD_FLAG_IN_LIST          (1 << 1)

  union
  {
    int val;                          /**< 事件参数，可以传递整形值 */

    int fd;                           /**< 事件参数，可以传递句柄 */

    struct timeval sands;             /**< 事件参数，可以传递定时器超时时间 */
  } u;

  short pfd_index;                    /**< poll数组索引，伪线程库内部使用，用户不要自己设置 */

  char name[SYD_THREAD_NAME_LEN + 1];  /**< 伪线程名 */
};

/* Macros.  */
#define SYD_THREAD_ARG(X)           ((X)->arg)       /**< 取伪线程的arg参数 */
#define SYD_THREAD_FD(X)            ((X)->u.fd)      /**< 取伪线程的句柄参数 */
#define SYD_THREAD_VAL(X)           ((X)->u.val)     /**< 取伪线程的整形参数 */
#define SYD_THREAD_TIME_VAL(X)      ((X)->u.sands)   /**< 取伪线程的超时时间参数 */
#define SYD_THREAD_GLOB(X)          ((X)->zg)        /**< 取伪线程的lib_global字段，zebos模块使用 */

/*
 * *< 用于判断该伪线程是否已添加到伪线程管理器中等待运行
 * 注意：如果是用户自己创建的thread，需要保证初始化的时候把thread结构体清0
 * 这个判断只适用于使用insert方式添加的伪线程，不适用于使用add方式添加的伪线程
 */
#define SYD_THREAD_IS_IN_LIST(THREAD) SYD_THREAD_CHECK_FLAG((THREAD)->flag, SYD_THREAD_FLAG_IN_LIST)

#define SYD_THREAD_READ_ON(master,thread,func,arg,sock) \
  do { \
    if (! thread) \
      thread = syd_thread_add_read (master, func, arg, sock); \
  } while (0)

#define SYD_THREAD_WRITE_ON(master,thread,func,arg,sock) \
  do { \
    if (! thread) \
      thread = syd_thread_add_write (master, func, arg, sock); \
  } while (0)

#define SYD_THREAD_TIMER_ON(master,thread,func,arg,time) \
  do { \
    if (! thread) \
      thread = syd_thread_add_timer (master, func, arg, time); \
  } while (0)

#define SYD_THREAD_OFF(thread) \
  do { \
    if (thread) \
      { \
        syd_thread_cancel (thread); \
        thread = NULL; \
      } \
  } while (0)

#define SYD_THREAD_READ_OFF(thread)   SYD_THREAD_OFF(thread)  /**< 删除一个socket的读线程 */
#define SYD_THREAD_WRITE_OFF(thread)  SYD_THREAD_OFF(thread)  /**< 删除一个socket的写线程 */
#define SYD_THREAD_TIMER_OFF(thread)  SYD_THREAD_OFF(thread)  /**< 删除一个定时器 */

/* Prototypes.  */

/**
 * @syd_thread_master_create 创建伪线程管理器
 *
 * @return 成功返回创建的伪线程管理器，否则返回空
 */
struct syd_thread_master *syd_thread_master_create (void);

/**
 * @syd_thread_master_finish 销毁伪线程管理器
 *
 * @param m 伪线程管理器
 * @return 无
 */
void syd_thread_master_finish (struct syd_thread_master *m);

/**
 * @syd_thread_use_poll 设置该伪线程管理器使用poll机制
 *
 * 如果进程的fd句柄数会超过1024，那么默认伪线程机制中使用的select将无法满足其需求。
 * 这种情况下，进程必须设置伪线程使用poll机制。<br>
 * 注: 只能设置一次是否使用poll机制，不能在运行过程中来回变换是否使用poll机制。
 *
 * @param m     伪线程管理器
 * @param max_fds  该伪线程管理器所涉及到的fd最大的数目，用来指明pollfd数组的大小
 * @return 成功返回0，否则返回负数
 */
int syd_thread_use_poll (struct syd_thread_master *m, short max_fds);

/**
 * @syd_thread_add_read 添加一个socket读线程
 *
 * 该伪线程为普通优先级。<br>
 * socket可读时，伪线程执行函数将被调用。
 * 在伪线程执行函数处理完后，如果还想继续监听socket，
 * 那么需要重新调用该函数添加一个socket读线程。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数，该函数中的syd_thread参数就是本次添加的伪线程
 * @param arg   伪线程arg参数，该值可以在执行函数func时从参数syd_thread里面取得
 * @param fd    伪线程socket句柄参数，该值可以在执行函数func时从参数syd_thread里面取得
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_read (struct syd_thread_master *m,
                                int (*func)(struct syd_thread *), void *arg,
                                int fd);

/**
 * @syd_thread_add_read_withname 添加一个带名字的socket读线程
 *
 * 该伪线程为普通优先级。
 * socket可读时，伪线程执行函数将被调用。
 * 在伪线程执行函数处理完后，如果还想继续监听socket，
 * 那么需要重新调用该函数添加一个socket读线程。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param fd    伪线程socket句柄参数
 * @param name  伪线程名
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_read_withname (struct syd_thread_master *m,
                                int (*func)(struct syd_thread *), void *arg,
                                int fd,
                                char *name);

/**
 * @syd_thread_add_read_high 添加一个socket高优先级读线程
 *
 * 该伪线程优先级高于普通socket读线程，但是仍然属于普通优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param fd    伪线程socket句柄参数
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_read_high (struct syd_thread_master *m,
                                     int (*func)(struct syd_thread *), void *arg,
                                     int fd);

/**
 * @syd_thread_add_read_high_withname 添加一个带名字的socket高优先级读线程
 *
 * 该伪线程优先级高于普通socket读线程，但是仍然属于普通优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param fd    伪线程socket句柄参数
 * @param name  伪线程名
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_read_high_withname (struct syd_thread_master *m,
                                     int (*func)(struct syd_thread *), void *arg,
                                     int fd,
                                     char *name);

/**
 * @syd_thread_insert_read 插入一个socket读线程
 *
 * 该接口与syd_thread_add_read类似，只不过伪线程由调用者自己创建并传入。
 * 只要调用者保证传入参数的正确，则该函数不会返回失败。参数正确性的说明在以下各参数说明中体现。
 *
 * @param m      伪线程管理器（不允许为空）
 * @param thread 伪线程（不允许为空）
 * @param func   伪线程执行函数（不允许为空），该函数中的syd_thread参数就是本次添加的伪线程
 * @param arg    伪线程arg参数，该值可以在执行函数func时从参数syd_thread里面取得
 * @param fd     伪线程socket句柄参数（必须是合法句柄），该值可以在执行函数func时从参数syd_thread里面取得
 * @param name   伪线程名（可以为空，如果不为空，长度不能超过10）
 * @return 成功返回0，否则返回负数
 */
int syd_thread_insert_read (struct syd_thread_master *m, struct syd_thread *thread,
                             int (*func) (struct syd_thread *),
                             void *arg, int fd, char *name);

/**
 * @syd_thread_insert_read_high 插入一个socket高优先级读线程
 *
 * 该接口与syd_thread_add_read_high类似，只不过伪线程由调用者自己创建并传入。
 * 只要调用者保证传入参数的正确，则该函数不会返回失败。参数正确性的说明在以下各参数说明中体现。
 *
 * @param m      伪线程管理器（不允许为空）
 * @param thread 伪线程（不允许为空）
 * @param func   伪线程执行函数（不允许为空）
 * @param arg    伪线程arg参数
 * @param fd     伪线程socket句柄参数（必须是合法句柄）
 * @param name   伪线程名（可以为空，如果不为空，长度不能超过10）
 * @return 成功返回0，否则返回负数
 */
int syd_thread_insert_read_high (struct syd_thread_master *m, struct syd_thread *thread,
                             int (*func) (struct syd_thread *),
                             void *arg, int fd, char *name);

/**
 * @syd_thread_add_write 添加一个socket写线程
 *
 * 该伪线程为普通优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param fd    伪线程socket句柄参数
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_write (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg,
                                 int fd);

/**
 * @syd_thread_add_write_withname 添加一个带名字的socket写线程
 *
 * 该伪线程为普通优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param fd    伪线程socket句柄参数
 * @param name  伪线程名
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_write_withname (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg,
                                 int fd,
                                 char *name);

/**
 * @syd_thread_insert_write 插入一个socket写线程
 *
 * 该接口与syd_thread_add_write类似，只不过伪线程由调用者自己创建并传入。
 * 只要调用者保证传入参数的正确，则该函数不会返回失败。参数正确性的说明在以下各参数说明中体现。
 *
 * @param m      伪线程管理器（不允许为空）
 * @param thread 伪线程（不允许为空）
 * @param func   伪线程执行函数（不允许为空）
 * @param arg    伪线程arg参数
 * @param fd     伪线程socket句柄参数（必须是合法句柄）
 * @param name   伪线程名（可以为空，如果不为空，长度不能超过10）
 * @return 成功返回0，否则返回负数
 */
int syd_thread_insert_write (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg,
        int fd, char *name);

/**
 * @syd_thread_add_timer 添加一个定时器
 *
 * 该伪线程为普通优先级。<br>
 * 定时器超时，伪线程执行函数将被调用。
 * 在伪线程执行函数处理完后，如果还想继续启动定时器，
 * 那么需要重新调用该函数添加一个定时器伪线程。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param timer 超时时间，单位为秒
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_timer (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg, long timer);

/**
 * @syd_thread_add_timer_withname 添加一个带名字的定时器
 *
 * 该伪线程为普通优先级。
 * 定时器超时，伪线程执行函数将被调用。
 * 在伪线程执行函数处理完后，如果还想继续启动定时器，
 * 那么需要重新调用该函数添加一个定时器伪线程。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param timer 超时时间，单位为秒
 * @param name  伪线程名
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_timer_withname (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg, long timer, char *name);

/**
 * @syd_thread_add_timer_timeval 添加一个定时器
 *
 * 该伪线程为普通优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param timer 超时时间
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_timer_timeval (struct syd_thread_master *m,
                                         int (*func)(struct syd_thread *),
                                         void *arg, struct timeval timer);

/**
 * @syd_thread_add_timer_timeval_withname 添加一个带名字的定时器
 *
 * 该伪线程为普通优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param timer 超时时间
 * @param name  伪线程名
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_timer_timeval_withname (struct syd_thread_master *m,
                                         int (*func)(struct syd_thread *),
                                         void *arg, struct timeval timer,
                                         char *name);

/**
 * @syd_thread_insert_timer 插入一个定时器
 *
 * 该接口与syd_thread_add_timer类似，只不过伪线程由调用者自己创建并传入。
 * 只要调用者保证传入参数的正确，则该函数不会返回失败。参数正确性的说明在以下各参数说明中体现。
 *
 * @param m      伪线程管理器（不允许为空）
 * @param thread 伪线程（不允许为空）
 * @param func   伪线程执行函数（不允许为空）
 * @param arg    伪线程arg参数
 * @param timer  超时时间
 * @param name   伪线程名（可以为空，如果不为空，长度不能超过10）
 * @return 成功返回0，否则返回负数
 */
int syd_thread_insert_timer (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, long timer, char *name);

/**
 * @syd_thread_insert_timer_timeval 插入一个定时器
 *
 * 该接口与syd_thread_add_timer_timeval类似，只不过伪线程由调用者自己创建并传入。
 * 只要调用者保证传入参数的正确，则该函数不会返回失败。参数正确性的说明在以下各参数说明中体现。
 *
 * @param m      伪线程管理器（不允许为空）
 * @param thread 伪线程（不允许为空）
 * @param func   伪线程执行函数（不允许为空）
 * @param arg    伪线程arg参数
 * @param timer  超时时间
 * @param name   伪线程名（可以为空，如果不为空，长度不能超过10）
 * @return 成功返回0，否则返回负数
 */
int syd_thread_insert_timer_timeval (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, struct timeval timer, char *name);

/**
 * @syd_thread_add_event 添加一个高优先级伪线程
 *
 * 该伪线程为高优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param val   伪线程整形参数
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_event (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg,
                                 int val);

/**
 * @syd_thread_add_event_withname 添加一个带名字的高优先级伪线程
 *
 * 该伪线程为高优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param val   伪线程整形参数
 * @param name  伪线程名
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_event_withname (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg,
                                 int val,
                                 char *name);

/**
 * @syd_thread_add_event_low 添加一个低优先级伪线程
 *
 * 该伪线程为低优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param val   伪线程整形参数
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_event_low (struct syd_thread_master *m,
                                     int (*func)(struct syd_thread *), void *arg,
                                     int val);

/**
 * @syd_thread_add_event_low_withname 添加一个带名字的低优先级伪线程
 *
 * 该伪线程为低优先级。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param val   伪线程整形参数
 * @param name  伪线程名
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_event_low_withname (struct syd_thread_master *m,
                                     int (*func)(struct syd_thread *), void *arg,
                                     int val,
                                     char *name);

/**
 * @syd_thread_insert_event 插入一个事件
 *
 * 该接口与syd_thread_add_event类似，只不过伪线程由调用者自己创建并传入。
 * 只要调用者保证传入参数的正确，则该函数不会返回失败。参数正确性的说明在以下各参数说明中体现。
 *
 * @param m      伪线程管理器（不允许为空）
 * @param thread 伪线程（不允许为空）
 * @param func   伪线程执行函数（不允许为空）
 * @param arg    伪线程arg参数
 * @param val    伪线程整形参数
 * @param name   伪线程名（可以为空，如果不为空，长度不能超过10）
 * @return 成功返回0，否则返回负数
 */
int syd_thread_insert_event (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, int val, char *name);

/**
 * @syd_thread_insert_event_low 插入一个事件
 *
 * 该接口与syd_thread_add_event_low类似，只不过伪线程由调用者自己创建并传入。
 * 只要调用者保证传入参数的正确，则该函数不会返回失败。参数正确性的说明在以下各参数说明中体现。
 *
 * @param m      伪线程管理器（不允许为空）
 * @param thread 伪线程（不允许为空）
 * @param func   伪线程执行函数（不允许为空）
 * @param arg    伪线程arg参数
 * @param val    伪线程整形参数
 * @param name   伪线程名（可以为空，如果不为空，长度不能超过10）
 * @return 成功返回0，否则返回负数
 */
int syd_thread_insert_event_low (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, int val, char *name);

/**
 * @syd_thread_add_read_pend 添加一个待处理消息伪线程
 *
 * 该伪线程为最高优先级。<br>
 * 只用于如下情况:有时候处理一个事件，可能需要循环等待某个消息返回，
 * 此时会读取到一些不是所等待的消息，
 * 这些消息不能直接被丢弃，需要为它们创建一个待处理消息伪线程，
 * 这些待处理消息伪线程会在下一次线程调度中优先执行。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param val   伪线程整形参数
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_read_pend (struct syd_thread_master *,
                                     int (*func) (struct syd_thread *), void *arg,
                                     int val);

/**
 * @syd_thread_add_read_pend_withname 添加一个带名字的待处理消息伪线程
 *
 * 该伪线程为最高优先级。
 * 只用于如下情况:有时候处理一个事件，可能需要循环等待某个消息返回，
 * 此时会读取到一些不是所等待的消息，
 * 这些消息不能直接被丢弃，需要为它们创建一个待处理消息伪线程，
 * 这些待处理消息伪线程会在下一次线程调度中优先执行。
 *
 * @param m     伪线程管理器
 * @param func  伪线程执行函数
 * @param arg   伪线程arg参数
 * @param val   伪线程整形参数
 * @param name  伪线程名
 * @return 成功返回添加的伪线程，否则返回空
 */
struct syd_thread *syd_thread_add_read_pend_withname (struct syd_thread_master *,
                                     int (*func) (struct syd_thread *), void *arg,
                                     int val,
                                     char *name);

/**
 * @syd_thread_insert_read_pend 插入一个待处理消息伪线程
 *
 * 该接口与syd_thread_add_read_pend类似，只不过伪线程由调用者自己创建并传入。
 * 只要调用者保证传入参数的正确，则该函数不会返回失败。参数正确性的说明在以下各参数说明中体现。
 *
 * @param m      伪线程管理器（不允许为空）
 * @param thread 伪线程（不允许为空）
 * @param func   伪线程执行函数（不允许为空）
 * @param arg    伪线程arg参数
 * @param val    伪线程整形参数
 * @param name   伪线程名（可以为空，如果不为空，长度不能超过10）
 * @return 成功返回0，否则返回负数
 */
int syd_thread_insert_read_pend (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, int val, char *name);

/**
 * @syd_thread_cancel 取消一个伪线程
 *
 * 如果所传入的伪线程是由应用程序自己创建，则该函数不会自动删除该伪线程；
 * 如果所传入的伪线程是由伪线程机制创建，则伪线程机制负责该伪线程的删除。
 *
 * @param thread     要取消的伪线程
 * @return     无
 */
void syd_thread_cancel (struct syd_thread *thread);

/**
 * @syd_thread_cancel_event 取消所有包含arg参数的伪线程
 *
 * 如果所传入的伪线程是由应用程序自己创建，则该函数不会自动删除该伪线程；
 * 如果所传入的伪线程是由伪线程机制创建，则伪线程机制负责该伪线程的删除。
 *
 * @param m          伪线程管理器
 * @param arg        arg参数
 * @return     无
 */
void syd_thread_cancel_event (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_cancel_event_low 取消所有包含arg参数的低优先级伪线程
 *
 * 如果所传入的伪线程是由应用程序自己创建，则该函数不会自动删除该伪线程；
 * 如果所传入的伪线程是由伪线程机制创建，则伪线程机制负责该伪线程的删除。
 *
 * @param m          伪线程管理器
 * @param arg        arg参数
 * @return     无
 */
void syd_thread_cancel_event_low (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_cancel_timer 取消所有包含arg参数的定时器
 *
 * 如果所传入的伪线程是由应用程序自己创建，则该函数不会自动删除该伪线程；
 * 如果所传入的伪线程是由伪线程机制创建，则伪线程机制负责该伪线程的删除。
 *
 * @param m          伪线程管理器
 * @param arg        arg参数
 * @return     无
 */
void syd_thread_cancel_timer (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_cancel_write 取消所有包含arg参数的写线程
 *
 * 如果所传入的伪线程是由应用程序自己创建，则该函数不会自动删除该伪线程；
 * 如果所传入的伪线程是由伪线程机制创建，则伪线程机制负责该伪线程的删除。
 *
 * @param m          伪线程管理器
 * @param arg        arg参数
 * @return     无
 */
void syd_thread_cancel_write (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_cancel_read 取消所有包含arg参数的读线程
 *
 * 如果所传入的伪线程是由应用程序自己创建，则该函数不会自动删除该伪线程；
 * 如果所传入的伪线程是由伪线程机制创建，则伪线程机制负责该伪线程的删除。
 *
 * @param m          伪线程管理器
 * @param arg        arg参数
 * @return     无
 */
void syd_thread_cancel_read (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_fetch 获取一个可运行伪线程
 *
 * @param m          伪线程管理器
 * @param fetch      可运行伪线程保存的指针
 * @return     成功返回可运行伪线程，否则返回空
 */
struct syd_thread *syd_thread_fetch (struct syd_thread_master *m, struct syd_thread *fetch);

/**
 * @syd_thread_execute 运行一个伪线程执行函数
 *
 * @param func       伪线程执行函数
 * @param arg        伪线程的arg参数
 * @param val        伪线程的整形参数
 * @return     返回空
 */
struct syd_thread *syd_thread_execute (int (*func)(struct syd_thread *), void *arg,
                                     int val);

/**
 * @syd_thread_call 运行一个伪线程
 *
 * @param thread     伪线程
 * @return     无
 */
void syd_thread_call (struct syd_thread *thread);

/**
 * @syd_thread_timer_remain_second 获取定时器剩余时间
 *
 * @param thread     定时器伪线程
 * @return     成功返回定时器剩余时间，否则返回0
 */
u_int32_t syd_thread_timer_remain_second (struct syd_thread *thread);

#ifdef HAVE_NGSA
void syd_thread_list_add (struct syd_thread_list *, struct syd_thread *);
void syd_thread_list_execute (struct syd_thread_master *, struct syd_thread_list *);
void syd_thread_list_clear (struct syd_thread_master *, struct syd_thread_list *);

struct syd_thread *syd_thread_get (struct syd_thread_master *, char,
                                 int (*) (struct syd_thread *), void *);
void syd_thread_cancel_event_by_asyd_func (struct syd_thread_master *,
                                         int (*)(struct syd_thread *), void *);
void syd_thread_cancel_by_asyd_func (struct syd_thread_master *,
                                int (*)(struct syd_thread *), void *);
void syd_thread_cancel_asyd_check_list (struct syd_thread_master *, struct syd_thread_list *list, void *arg);
void syd_thread_cancel_asyd_check (struct syd_thread_master *, void *arg);
void syd_thread_cancel_by_arg (struct syd_thread_master *, void *arg);
void syd_thread_cancel_invalid_fd(struct syd_thread_master *, struct syd_thread_list *list);
#endif

/**
 * @syd_thread_disp_thread_master 打印伪线程管理器信息
 *
 * @param m          伪线程管理器
 * @return     返回空
 */
void
syd_thread_disp_thread_master (struct syd_thread_master *m);

/**
 * @syd_thread_disp_thread_queue 打印伪线程队列信息
 *
 * @param m          伪线程管理器
 * @param queue_type 伪线程队列类型
 * @return     返回空
 */
void
syd_thread_disp_thread_queue (struct syd_thread_master *m, int queue_type);

/**
 * @syd_thread_disp_thread 打印伪线程信息
 *
 * @param thread          伪线程
 * @return     返回空
 */
void
syd_thread_disp_thread (struct syd_thread *thread);

/**
 * @syd_thread_disp_thread_byname 打印指定名字的伪线程信息
 *
 * @param m          伪线程管理器
 * @param name       伪线程名
 * @return     返回空
 */
void
syd_thread_disp_thread_byname (struct syd_thread_master *m, char *name);

/**
 * @syd_thread_debug_interface 打开伪线程接口调试开关
 *
 * @param m             伪线程管理器
 * @param debug_switch  开关状态(SYD_THREAD_DEBUG_OFF/SYD_THREAD_DEBUG_ON)
 * @return     返回空
 */
void
syd_thread_debug_interface (struct syd_thread_master *m, int debug_switch);

/**
 * @syd_thread_debug_procedure 打开伪线程过程调试开关
 *
 * @param m             伪线程管理器
 * @param debug_switch  开关状态(SYD_THREAD_DEBUG_OFF/SYD_THREAD_DEBUG_ON)
 * @return     返回空
 */
void
syd_thread_debug_procedure (struct syd_thread_master *m, int debug_switch);

#endif /* _SYD_THREAD_H */

