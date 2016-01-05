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
  SemaType(uint32_t count) {
    int ret = sem_init(&sema_, 0, count);
    if (ret != 0) throw std::runtime_error("sem_init fail");
  }

  ~SemaType() {
    sem_destroy(&sema_);
  }

  bool Post() { return (0 == sem_post(&sema_)); }

  bool Wait(double sec) {
    if (sec < 0) return (0 == sem_wait(&sema_));

    if (sec > 0) {
      struct timespec ts;
      
    }
  }

private:
  SemaType(const SemaType &);
  SemaType& operator=(const SemaType &);

private:
  sem_t sema_;
};

}//end-cromwell.

#endif
