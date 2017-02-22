#ifndef _LIB_H
#define _LIB_H

#include <sys/time.h>

typedef unsigned int u_int32_t;

#define XCALLOC(type, size)      calloc (1, size)
#define XMALLOC(type, size)      malloc (size)
#define XFREE(type,ptr)          free (ptr)

#define SYD_THREAD_INVALID_INDEX (-1)

#define SYD_THREAD_SELECT_FD_MAX (1024)  /* select机制所支持的最大fd值 */

/* 函数声明 */
void syd_time_tzcurrent (struct timeval *tv, struct timezone *tz);

//#define	ULONG_MAX	0xffffffffUL	/* max value in unsigned long	*/
#define	HZ	100
#define WRAPAROUND_VALUE (ULONG_MAX / HZ + 1) /* HZ = frequency of ticks per second. */

enum memory_type
{
  SYD_MTYPE_TMP = 0,        /* Must always be first and should be zero. */

  /* Thread */
  SYD_MTYPE_THREAD_MASTER,
  SYD_MTYPE_THREAD,

  SYD_MTYPE_POLLFD,

  SYD_MTYPE_MAX		/* Must be last & should be largest. */
};
#endif /*_LIB_H*/


