#ifndef __CROMWELL_LOCK_H
#define __CROMWELL_LOCK_H

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "times.h"

namespace cromwell {

class NullLock {
public:
  inline bool AcquireReadLock() {
    return true;
  }

  inline bool ReleaseReadLock() {
    return true;
  }

  inline bool AcquireWriteLock() {
    return true;
  }

  inline bool ReleaseWriteLock() {
    return true;
  }

  inline bool TryAcquireReadLock(double /* sec */) {
    return true;
  }

  inline bool TryAcquireWriteLock(double /* sec */) {
    return true;
  }
};

class ReadWriteLock {
 public:
  ReadWriteLock() {
    pthread_rwlock_init(&rwlock_, NULL);
  }

  ~ReadWriteLock() {
    pthread_rwlock_destroy(&rwlock_);
  }

  inline bool AcquireReadLock() {
    return (0 == pthread_rwlock_rdlock(&rwlock_));
  }

  inline bool AcquireWriteLock() {
    return (0 == pthread_rwlock_wrlock(&rwlock_));
  }

  inline bool ReleaseReadLock() {
    return (0 == pthread_rwlock_unlock(&rwlock_));
  }

  inline bool ReleaseWriteLock() {
    return (0 == pthread_rwlock_unlock(&rwlock_));
  }

  inline bool TryAcquireReadLock(double seconds) {
    int r = pthread_rwlock_tryrdlock(&rwlock_);
    if (0 == r) return true;

    if (seconds > 0) {
      struct timespec ts;
      TimespecConvert(seconds, &ts);
      r = pthread_rwlock_timedrdlock(&rwlock_, &ts);
    }
    return (0 == r);
  }

  inline bool TryAcquireWriteLock(double seconds) {
    int r = pthread_rwlock_trywrlock(&rwlock_);
    if (0 == r) return true;

    if (seconds > 0) {
      struct timespec ts;
      TimespecConvert(seconds, &ts);
      r = pthread_rwlock_timedwrlock(&rwlock_, &ts);
    }
    return (0 == r);
  }

 private:
  ReadWriteLock(const ReadWriteLock &);
  ReadWriteLock& operator = (const ReadWriteLock &);

  pthread_rwlock_t rwlock_;
};


template<class LockType>
class ScopedReadLock {
 public:
  ScopedReadLock(LockType& lock): lock_(&lock) {
    lock_->AcquireReadLock();
  }
  ~ScopedReadLock() {
    this->Reset();
  }
  inline void Reset() {
    if (lock_) {
      lock_->ReleaseReadLock();
      lock_ = NULL;
    }
  }

 private:
  LockType* lock_;
};


template<class LockType>
class ScopedWriteLock {
 public:
  ScopedWriteLock(LockType& lock): lock_(&lock) {
    lock_->AcquireWriteLock();
  }
  ~ScopedWriteLock() {
    this->Reset();
  }
  inline void Reset() {
    if (lock_) {
      lock_->ReleaseWriteLock();
      lock_ = NULL;
    }
  }

 private:
  LockType* lock_;
};

typedef ScopedReadLock<ReadWriteLock> ReadLocker;
typedef ScopedWriteLock<ReadWriteLock> WriteLocker;

}//end-cromwell.

#endif
