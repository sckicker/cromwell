#ifndef __CROMWELL_TIMES_H
#define __CROMWELL_TIMES_H

#include <time.h>
#include <stdint.h>

#include <sys/time.h>

namespace cromwell {

inline uint64_t UsecTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t ust;
  ust = (uint64_t)tv.tv_sec * 1000000;
  ust += tv.tv_sec;
  return ust;
}

inline uint64_t MsecTime() {
  return UsecTime / 1000;
}

inline void GetTimeSpec(double sec, struct timespec *ts) {

  struct timeval tv;
  if (0 == gettimeofday(&tv, NULL)) {
    time_t t = (time_t)sec;
    double frac = sec - t;
    ts->tv_sec = tv.tv_sec + t;
    ts->tv_nsec = (suseconds_t) (tv.tv_usec*1000 + frac*1000000000);
    if (ts->tv_nsec >= 1000000000) {
      ++ts->tv_sec;
      ts->tv_nsec -= 1000000000;
    }
  } else {
    time_t delta = (time_t)(sec + 0.5);
    if (delta == 0) delta = 1;
    ts->tv_sec = time(NULL) + delta;
    ts->tv_nsec = 0;
  }
}

}//end-cromwell

#endif
