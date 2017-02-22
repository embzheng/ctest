#include <sys/time.h>
#include <time.h>    /* clock_gettime */
#include <stddef.h>  /* NULL宏 */
#if 0
#include <sys/sysinfo.h>
#endif
#include "lib.h"

#if 0
/* Static function to get current sec and usec.  */
static int
system_uptime (struct timeval *tv, struct timezone *tz)
{
  static unsigned long prev = 0;
  static unsigned long wraparound_count = 0;
  unsigned long uptime;
  static long base = 0;
  static long offset = 0;
  long leap;
  long diff;
#if 1
  struct sysinfo info;

  /* Get sysinfo.  */
  if (sysinfo (&info) < 0)
    return -1;

  /* Check for wraparound. */
  if (prev > info.uptime)
    wraparound_count++;

  /* System uptime.  */
  uptime = wraparound_count * WRAPAROUND_VALUE + info.uptime;
  prev = info.uptime;
#else
  /* Check for wraparound. */
  uptime = clock();
  if (prev > uptime)
    wraparound_count++;

  /* System uptime.  */
  prev = uptime;
  uptime = wraparound_count * WRAPAROUND_VALUE + uptime;

#endif
  /* Get tv_sec and tv_usec.  */
  gettimeofday (tv, tz);

  /* Deffernce between gettimeofday sec and uptime.  */
  leap = tv->tv_sec - uptime;

  /* Remember base diff for adjustment.  */
  if (! base)
    base = leap;

  /* Basically we use gettimeofday's return value because it is the
     only way to get required granularity.  But when diff is very
     different we adjust the value using base value.  */
  diff = (leap - base) + offset;

  /* When system time go forward than 2 sec.  */
  if (diff > 2 || diff < -2)
    offset -= diff;

  /* Adjust second.  */
  tv->tv_sec += offset;

  return 0;
}

#else
static int
system_uptime (struct timeval *tv, struct timezone *tz)
{
  struct timespec ts;

  if (tz != NULL)
    {
      /* 取当前的绝对时间 */
      gettimeofday (tv, tz);
    }
  else if (tv != NULL)
    {
      /* 获取系统起机时间 */
      clock_gettime (CLOCK_MONOTONIC, &ts);

      tv->tv_sec = ts.tv_sec;
      tv->tv_usec = ts.tv_nsec / 1000;
    }

  return 0;
}
#endif

void
syd_time_tzcurrent (struct timeval *tv,
		    struct timezone *tz)
{
  system_uptime (tv, tz);
  return;
}

