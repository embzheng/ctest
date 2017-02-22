/* Copyright (C) 2001-2003 IP Infusion, Inc. All Rights Reserved. */

#ifndef _SYD_THREAD_H
#define _SYD_THREAD_H

//#include <syd_lib/syd_syslog.h>
#include <stdio.h>
#include "lib.h"

/**
 * @defgroup    SERVICE_SYD_THREAD     Native API
 *
 * α�߳�API�ӿڡ�ͷ�ļ�<libpub/syd_thread/syd_thread.h>
 *
 * <p>
 * α�̲߳���ʵ�������ϵĲ���ϵͳ�̣߳����������ڲ���ϵͳ�ĵ��ȶ����С�
 * ����ʹ��α�̻߳��Ƶ�ģ����˵��һ������¸�ģ��ᴴ��һ����ʵ���̣߳�
 * Ȼ��������߳��н���α�̵߳ĵ��ȡ�<br>
 * ����α�̻߳����е��̲߳������������ϵĲ���ϵͳ�̣߳����ĳ��α�̹߳�ס��ʹ��α�̵߳�����
 * �޷��ټ������У�Ҳ�ͻ������Ӧ�Ĳ���ϵͳ���̻��̹߳�ס����ˣ����Ҫ�����ϵͳ���̻��̲߳���ס��
 * ��ôα�̵߳Ĵ����в����й������������ƿ��ǵ�ʱ��ͨ��α�̵߳��Ȼ���һ������¿�������
 * α�̵߳Ĵ����������磬����socket��д����������ʹ��ԭ�ȵ�ֱ�ӳ���ȥ������д�ķ�ʽ��
 * �������һ������дα�̣߳�α�̵߳��Ȼ������ж�socket�ɶ����߿�д��ʱ��Ž��ж���д������
 * <p>
 * ����ʵ�ʲ���ϵͳ�߳�һ����α�߳�Ҳͬ���������ȼ���һ���ԣ��߳����ȼ���Ҫ�ǿ�����ʹ�����и���
 * ��ѡ�������Ÿ����¼��ĵ���˳��α�̻߳��ƾ����������ȼ�:
 * ������ȼ��������ȼ�����ͨ���ȼ��������ȼ���
 * <p>
 * α�̻߳�����һ������ѭ���Ļ��ƣ�Ҳ����˵ʹ��α�̵߳�ģ�齫һֱѭ�����ҿ������е�α�̣߳�
 * �ҵ������е��̺߳������ִ�С���ˣ���ϵͳ��ʼ����ʱ��������Ҫ���һ��α�̣߳��������
 * ��������ѭ��������û�п����е��̣߳���һֱ�޷�����µ�α�߳̽�ȥ��<br>
 * <b>ע��:α�̻߳���ֻ����һ������ϵͳ�̶߳�����в��������ܹ����������ϵĲ���ϵͳ�߳�ͬʱ��һ��
 * α�̹߳������е�α�߳̽�����ӡ�ɾ���ȡ�</b>
 * <p>
 * �����Ǹ���򵥵�ʹ��α�̵߳����ӣ�
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


/* �����ȡע�͵Ķ��� */
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
 /* ����Ϊ�����ȡע�͵Ķ��� */

/**
 * α�̹߳�����.
 * ��ʹ��α�̻߳���ʱ�����봴��һ��α�̹߳����������ڹ���α�̡߳�
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

  /* ʹ��poll���� */
  struct pollfd *pfds;      /**< pollfd���� */
  short nfds;               /**< pollfd��Ч�����С��α�߳̿��ڲ�ʹ�ã��û���Ҫ�Լ����� */
  short pfds_size;          /**< pollfd�����С��α�߳̿��ڲ�ʹ�ã��û���Ҫ�Լ����� */

  u_int32_t alloc;
  int debug;
#define SYD_THREAD_DEBUG_INTERFACE_FLAG     (1 << 0)
#define SYD_THREAD_DEBUG_PROCEDURE_FLAG     (1 << 1)

  u_int32_t flag;
#define SYD_THREAD_IS_FIRST_FETCH (1 << 0)
#define SYD_THREAD_USE_POLL (1 << 1)

  int tid;   /*�߳�ID*/

  struct syd_thread *t_recover;
};

#define SYD_THREAD_NAME_LEN          10  /* α�߳�����󳤶� */

/**
 * α�̡߳�
 */
struct syd_thread
{
  struct syd_thread *next;
  struct syd_thread *prev;

  struct syd_thread_master *master;

  int (*func) (struct syd_thread *);   /**< α�߳�ִ�к��� */

  void *zg;                           /**< zebosģ��ʹ�õ�lib_globals */

  void *arg;                          /**< �¼�������һ�㴫�ݽṹ��ָ�� */

  char type;                          /**< α�߳����ͣ�α�߳̿�ʹ�ã��û���Ҫ�Լ����� */

  char priority;                      /**< α�߳����ȼ������Լ����ã�ͨ�����벻ͬ����α�߳�ʱ��α�߳̿��Զ����� */
#define SYD_THREAD_PRIORITY_HIGH         0
#define SYD_THREAD_PRIORITY_MIDDLE       1
#define SYD_THREAD_PRIORITY_LOW          2

  char index;                         /**< α�̶߳�ʱ������������α�߳̿�ʹ�ã��û���Ҫ�Լ����� */

  char flag;                          /**< α�̱߳�־��α�߳̿�ʹ�ã��û���Ҫ�Լ����� */
#define SYD_THREAD_FLAG_CRT_BY_USR       (1 << 0)
#define SYD_THREAD_FLAG_IN_LIST          (1 << 1)

  union
  {
    int val;                          /**< �¼����������Դ�������ֵ */

    int fd;                           /**< �¼����������Դ��ݾ�� */

    struct timeval sands;             /**< �¼����������Դ��ݶ�ʱ����ʱʱ�� */
  } u;

  short pfd_index;                    /**< poll����������α�߳̿��ڲ�ʹ�ã��û���Ҫ�Լ����� */

  char name[SYD_THREAD_NAME_LEN + 1];  /**< α�߳��� */
};

/* Macros.  */
#define SYD_THREAD_ARG(X)           ((X)->arg)       /**< ȡα�̵߳�arg���� */
#define SYD_THREAD_FD(X)            ((X)->u.fd)      /**< ȡα�̵߳ľ������ */
#define SYD_THREAD_VAL(X)           ((X)->u.val)     /**< ȡα�̵߳����β��� */
#define SYD_THREAD_TIME_VAL(X)      ((X)->u.sands)   /**< ȡα�̵߳ĳ�ʱʱ����� */
#define SYD_THREAD_GLOB(X)          ((X)->zg)        /**< ȡα�̵߳�lib_global�ֶΣ�zebosģ��ʹ�� */

/*
 * *< �����жϸ�α�߳��Ƿ�����ӵ�α�̹߳������еȴ�����
 * ע�⣺������û��Լ�������thread����Ҫ��֤��ʼ����ʱ���thread�ṹ����0
 * ����ж�ֻ������ʹ��insert��ʽ��ӵ�α�̣߳���������ʹ��add��ʽ��ӵ�α�߳�
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

#define SYD_THREAD_READ_OFF(thread)   SYD_THREAD_OFF(thread)  /**< ɾ��һ��socket�Ķ��߳� */
#define SYD_THREAD_WRITE_OFF(thread)  SYD_THREAD_OFF(thread)  /**< ɾ��һ��socket��д�߳� */
#define SYD_THREAD_TIMER_OFF(thread)  SYD_THREAD_OFF(thread)  /**< ɾ��һ����ʱ�� */

/* Prototypes.  */

/**
 * @syd_thread_master_create ����α�̹߳�����
 *
 * @return �ɹ����ش�����α�̹߳����������򷵻ؿ�
 */
struct syd_thread_master *syd_thread_master_create (void);

/**
 * @syd_thread_master_finish ����α�̹߳�����
 *
 * @param m α�̹߳�����
 * @return ��
 */
void syd_thread_master_finish (struct syd_thread_master *m);

/**
 * @syd_thread_use_poll ���ø�α�̹߳�����ʹ��poll����
 *
 * ������̵�fd������ᳬ��1024����ôĬ��α�̻߳�����ʹ�õ�select���޷�����������
 * ��������£����̱�������α�߳�ʹ��poll���ơ�<br>
 * ע: ֻ������һ���Ƿ�ʹ��poll���ƣ����������й��������ر任�Ƿ�ʹ��poll���ơ�
 *
 * @param m     α�̹߳�����
 * @param max_fds  ��α�̹߳��������漰����fd������Ŀ������ָ��pollfd����Ĵ�С
 * @return �ɹ�����0�����򷵻ظ���
 */
int syd_thread_use_poll (struct syd_thread_master *m, short max_fds);

/**
 * @syd_thread_add_read ���һ��socket���߳�
 *
 * ��α�߳�Ϊ��ͨ���ȼ���<br>
 * socket�ɶ�ʱ��α�߳�ִ�к����������á�
 * ��α�߳�ִ�к������������������������socket��
 * ��ô��Ҫ���µ��øú������һ��socket���̡߳�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к������ú����е�syd_thread�������Ǳ�����ӵ�α�߳�
 * @param arg   α�߳�arg��������ֵ������ִ�к���funcʱ�Ӳ���syd_thread����ȡ��
 * @param fd    α�߳�socket�����������ֵ������ִ�к���funcʱ�Ӳ���syd_thread����ȡ��
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_read (struct syd_thread_master *m,
                                int (*func)(struct syd_thread *), void *arg,
                                int fd);

/**
 * @syd_thread_add_read_withname ���һ�������ֵ�socket���߳�
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 * socket�ɶ�ʱ��α�߳�ִ�к����������á�
 * ��α�߳�ִ�к������������������������socket��
 * ��ô��Ҫ���µ��øú������һ��socket���̡߳�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_read_withname (struct syd_thread_master *m,
                                int (*func)(struct syd_thread *), void *arg,
                                int fd,
                                char *name);

/**
 * @syd_thread_add_read_high ���һ��socket�����ȼ����߳�
 *
 * ��α�߳����ȼ�������ͨsocket���̣߳�������Ȼ������ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_read_high (struct syd_thread_master *m,
                                     int (*func)(struct syd_thread *), void *arg,
                                     int fd);

/**
 * @syd_thread_add_read_high_withname ���һ�������ֵ�socket�����ȼ����߳�
 *
 * ��α�߳����ȼ�������ͨsocket���̣߳�������Ȼ������ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_read_high_withname (struct syd_thread_master *m,
                                     int (*func)(struct syd_thread *), void *arg,
                                     int fd,
                                     char *name);

/**
 * @syd_thread_insert_read ����һ��socket���߳�
 *
 * �ýӿ���syd_thread_add_read���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ����ú����е�syd_thread�������Ǳ�����ӵ�α�߳�
 * @param arg    α�߳�arg��������ֵ������ִ�к���funcʱ�Ӳ���syd_thread����ȡ��
 * @param fd     α�߳�socket��������������ǺϷ����������ֵ������ִ�к���funcʱ�Ӳ���syd_thread����ȡ��
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int syd_thread_insert_read (struct syd_thread_master *m, struct syd_thread *thread,
                             int (*func) (struct syd_thread *),
                             void *arg, int fd, char *name);

/**
 * @syd_thread_insert_read_high ����һ��socket�����ȼ����߳�
 *
 * �ýӿ���syd_thread_add_read_high���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param fd     α�߳�socket��������������ǺϷ������
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int syd_thread_insert_read_high (struct syd_thread_master *m, struct syd_thread *thread,
                             int (*func) (struct syd_thread *),
                             void *arg, int fd, char *name);

/**
 * @syd_thread_add_write ���һ��socketд�߳�
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_write (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg,
                                 int fd);

/**
 * @syd_thread_add_write_withname ���һ�������ֵ�socketд�߳�
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param fd    α�߳�socket�������
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_write_withname (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg,
                                 int fd,
                                 char *name);

/**
 * @syd_thread_insert_write ����һ��socketд�߳�
 *
 * �ýӿ���syd_thread_add_write���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param fd     α�߳�socket��������������ǺϷ������
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int syd_thread_insert_write (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg,
        int fd, char *name);

/**
 * @syd_thread_add_timer ���һ����ʱ��
 *
 * ��α�߳�Ϊ��ͨ���ȼ���<br>
 * ��ʱ����ʱ��α�߳�ִ�к����������á�
 * ��α�߳�ִ�к��������������������������ʱ����
 * ��ô��Ҫ���µ��øú������һ����ʱ��α�̡߳�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param timer ��ʱʱ�䣬��λΪ��
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_timer (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg, long timer);

/**
 * @syd_thread_add_timer_withname ���һ�������ֵĶ�ʱ��
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 * ��ʱ����ʱ��α�߳�ִ�к����������á�
 * ��α�߳�ִ�к��������������������������ʱ����
 * ��ô��Ҫ���µ��øú������һ����ʱ��α�̡߳�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param timer ��ʱʱ�䣬��λΪ��
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_timer_withname (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg, long timer, char *name);

/**
 * @syd_thread_add_timer_timeval ���һ����ʱ��
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param timer ��ʱʱ��
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_timer_timeval (struct syd_thread_master *m,
                                         int (*func)(struct syd_thread *),
                                         void *arg, struct timeval timer);

/**
 * @syd_thread_add_timer_timeval_withname ���һ�������ֵĶ�ʱ��
 *
 * ��α�߳�Ϊ��ͨ���ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param timer ��ʱʱ��
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_timer_timeval_withname (struct syd_thread_master *m,
                                         int (*func)(struct syd_thread *),
                                         void *arg, struct timeval timer,
                                         char *name);

/**
 * @syd_thread_insert_timer ����һ����ʱ��
 *
 * �ýӿ���syd_thread_add_timer���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param timer  ��ʱʱ��
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int syd_thread_insert_timer (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, long timer, char *name);

/**
 * @syd_thread_insert_timer_timeval ����һ����ʱ��
 *
 * �ýӿ���syd_thread_add_timer_timeval���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param timer  ��ʱʱ��
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int syd_thread_insert_timer_timeval (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, struct timeval timer, char *name);

/**
 * @syd_thread_add_event ���һ�������ȼ�α�߳�
 *
 * ��α�߳�Ϊ�����ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_event (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg,
                                 int val);

/**
 * @syd_thread_add_event_withname ���һ�������ֵĸ����ȼ�α�߳�
 *
 * ��α�߳�Ϊ�����ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_event_withname (struct syd_thread_master *m,
                                 int (*func)(struct syd_thread *), void *arg,
                                 int val,
                                 char *name);

/**
 * @syd_thread_add_event_low ���һ�������ȼ�α�߳�
 *
 * ��α�߳�Ϊ�����ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_event_low (struct syd_thread_master *m,
                                     int (*func)(struct syd_thread *), void *arg,
                                     int val);

/**
 * @syd_thread_add_event_low_withname ���һ�������ֵĵ����ȼ�α�߳�
 *
 * ��α�߳�Ϊ�����ȼ���
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_event_low_withname (struct syd_thread_master *m,
                                     int (*func)(struct syd_thread *), void *arg,
                                     int val,
                                     char *name);

/**
 * @syd_thread_insert_event ����һ���¼�
 *
 * �ýӿ���syd_thread_add_event���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param val    α�߳����β���
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int syd_thread_insert_event (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, int val, char *name);

/**
 * @syd_thread_insert_event_low ����һ���¼�
 *
 * �ýӿ���syd_thread_add_event_low���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param val    α�߳����β���
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int syd_thread_insert_event_low (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, int val, char *name);

/**
 * @syd_thread_add_read_pend ���һ����������Ϣα�߳�
 *
 * ��α�߳�Ϊ������ȼ���<br>
 * ֻ�����������:��ʱ����һ���¼���������Ҫѭ���ȴ�ĳ����Ϣ���أ�
 * ��ʱ���ȡ��һЩ�������ȴ�����Ϣ��
 * ��Щ��Ϣ����ֱ�ӱ���������ҪΪ���Ǵ���һ����������Ϣα�̣߳�
 * ��Щ��������Ϣα�̻߳�����һ���̵߳���������ִ�С�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_read_pend (struct syd_thread_master *,
                                     int (*func) (struct syd_thread *), void *arg,
                                     int val);

/**
 * @syd_thread_add_read_pend_withname ���һ�������ֵĴ�������Ϣα�߳�
 *
 * ��α�߳�Ϊ������ȼ���
 * ֻ�����������:��ʱ����һ���¼���������Ҫѭ���ȴ�ĳ����Ϣ���أ�
 * ��ʱ���ȡ��һЩ�������ȴ�����Ϣ��
 * ��Щ��Ϣ����ֱ�ӱ���������ҪΪ���Ǵ���һ����������Ϣα�̣߳�
 * ��Щ��������Ϣα�̻߳�����һ���̵߳���������ִ�С�
 *
 * @param m     α�̹߳�����
 * @param func  α�߳�ִ�к���
 * @param arg   α�߳�arg����
 * @param val   α�߳����β���
 * @param name  α�߳���
 * @return �ɹ�������ӵ�α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_add_read_pend_withname (struct syd_thread_master *,
                                     int (*func) (struct syd_thread *), void *arg,
                                     int val,
                                     char *name);

/**
 * @syd_thread_insert_read_pend ����һ����������Ϣα�߳�
 *
 * �ýӿ���syd_thread_add_read_pend���ƣ�ֻ����α�߳��ɵ������Լ����������롣
 * ֻҪ�����߱�֤�����������ȷ����ú������᷵��ʧ�ܡ�������ȷ�Ե�˵�������¸�����˵�������֡�
 *
 * @param m      α�̹߳�������������Ϊ�գ�
 * @param thread α�̣߳�������Ϊ�գ�
 * @param func   α�߳�ִ�к�����������Ϊ�գ�
 * @param arg    α�߳�arg����
 * @param val    α�߳����β���
 * @param name   α�߳���������Ϊ�գ������Ϊ�գ����Ȳ��ܳ���10��
 * @return �ɹ�����0�����򷵻ظ���
 */
int syd_thread_insert_read_pend (struct syd_thread_master *m, struct syd_thread *thread,
        int (*func)(struct syd_thread *), void *arg, int val, char *name);

/**
 * @syd_thread_cancel ȡ��һ��α�߳�
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param thread     Ҫȡ����α�߳�
 * @return     ��
 */
void syd_thread_cancel (struct syd_thread *thread);

/**
 * @syd_thread_cancel_event ȡ�����а���arg������α�߳�
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void syd_thread_cancel_event (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_cancel_event_low ȡ�����а���arg�����ĵ����ȼ�α�߳�
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void syd_thread_cancel_event_low (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_cancel_timer ȡ�����а���arg�����Ķ�ʱ��
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void syd_thread_cancel_timer (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_cancel_write ȡ�����а���arg������д�߳�
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void syd_thread_cancel_write (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_cancel_read ȡ�����а���arg�����Ķ��߳�
 *
 * ����������α�߳�����Ӧ�ó����Լ���������ú��������Զ�ɾ����α�̣߳�
 * ����������α�߳�����α�̻߳��ƴ�������α�̻߳��Ƹ����α�̵߳�ɾ����
 *
 * @param m          α�̹߳�����
 * @param arg        arg����
 * @return     ��
 */
void syd_thread_cancel_read (struct syd_thread_master *m, void *arg);

/**
 * @syd_thread_fetch ��ȡһ��������α�߳�
 *
 * @param m          α�̹߳�����
 * @param fetch      ������α�̱߳����ָ��
 * @return     �ɹ����ؿ�����α�̣߳����򷵻ؿ�
 */
struct syd_thread *syd_thread_fetch (struct syd_thread_master *m, struct syd_thread *fetch);

/**
 * @syd_thread_execute ����һ��α�߳�ִ�к���
 *
 * @param func       α�߳�ִ�к���
 * @param arg        α�̵߳�arg����
 * @param val        α�̵߳����β���
 * @return     ���ؿ�
 */
struct syd_thread *syd_thread_execute (int (*func)(struct syd_thread *), void *arg,
                                     int val);

/**
 * @syd_thread_call ����һ��α�߳�
 *
 * @param thread     α�߳�
 * @return     ��
 */
void syd_thread_call (struct syd_thread *thread);

/**
 * @syd_thread_timer_remain_second ��ȡ��ʱ��ʣ��ʱ��
 *
 * @param thread     ��ʱ��α�߳�
 * @return     �ɹ����ض�ʱ��ʣ��ʱ�䣬���򷵻�0
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
 * @syd_thread_disp_thread_master ��ӡα�̹߳�������Ϣ
 *
 * @param m          α�̹߳�����
 * @return     ���ؿ�
 */
void
syd_thread_disp_thread_master (struct syd_thread_master *m);

/**
 * @syd_thread_disp_thread_queue ��ӡα�̶߳�����Ϣ
 *
 * @param m          α�̹߳�����
 * @param queue_type α�̶߳�������
 * @return     ���ؿ�
 */
void
syd_thread_disp_thread_queue (struct syd_thread_master *m, int queue_type);

/**
 * @syd_thread_disp_thread ��ӡα�߳���Ϣ
 *
 * @param thread          α�߳�
 * @return     ���ؿ�
 */
void
syd_thread_disp_thread (struct syd_thread *thread);

/**
 * @syd_thread_disp_thread_byname ��ӡָ�����ֵ�α�߳���Ϣ
 *
 * @param m          α�̹߳�����
 * @param name       α�߳���
 * @return     ���ؿ�
 */
void
syd_thread_disp_thread_byname (struct syd_thread_master *m, char *name);

/**
 * @syd_thread_debug_interface ��α�߳̽ӿڵ��Կ���
 *
 * @param m             α�̹߳�����
 * @param debug_switch  ����״̬(SYD_THREAD_DEBUG_OFF/SYD_THREAD_DEBUG_ON)
 * @return     ���ؿ�
 */
void
syd_thread_debug_interface (struct syd_thread_master *m, int debug_switch);

/**
 * @syd_thread_debug_procedure ��α�̹߳��̵��Կ���
 *
 * @param m             α�̹߳�����
 * @param debug_switch  ����״̬(SYD_THREAD_DEBUG_OFF/SYD_THREAD_DEBUG_ON)
 * @return     ���ؿ�
 */
void
syd_thread_debug_procedure (struct syd_thread_master *m, int debug_switch);

#endif /* _SYD_THREAD_H */

