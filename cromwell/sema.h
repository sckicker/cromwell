#ifndef __CROMWELL_SEMA_H
#define __CROMWELL_SEMA_H

#include <stdlib.h>
#include <stdint.h>
#include <stdexcept>
#include <semaphore.h>

#include "timespec.h"

namespace cromwell {

class SemaType {
public:
  explicit SemaType(uint32_t count) {
    if (0 != sem_init(&sema_, 0, count)) {
      throw std::runtime_error("sem_init fail");
    }
  }

  ~SemaType() {
    sem_destroy(&sema_);
  }

  bool Post() { return (0 == sem_post(&sema_)); }

  bool Wait(double sec) {
    if (sec < 0) {
      return (0 == sem_wait(&sema_));
    }
    if (sec > 0) {
      struct timespec ts;
      GetTimeSpec(sec, &ts);
      return (sem_timedwait(&sema_, &ts) == 0);
    }//end-if
    return (sem_trywait(&sema_) == 0);
  }

  bool TryWait() {
    return sem_trywait(&sema_) == 0;
  }

private:
  SemaType(const SemaType &);
  SemaType& operator=(const SemaType &);

private:
  sem_t sema_;
};

}//end-cromwell.

#endif
