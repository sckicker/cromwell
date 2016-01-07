#ifndef __CROMWELL_SIMPLE_MEMPOOL_H
#define __CROMWELL_SIMPLE_MEMPOOL_H

#include <cstdlib>
#include "mutex.h"

namespace cromwell {

template <int size, class LockType>
class SimpleMemPool {
public:
  SimpleMemPool()
  : head_(NULL) {
    ASSERT(sizeof(void*) <= size);
  }

  ~SimpleMemPool() {
    FreeAll();
  }

  static SimpleMemPool<size, LockType>& Instance() {
    static SimpleMemPool<size, LockType> instance;
    return instance;
  }

  void* Allocate();
  void Release(void* p);

private:
  SimpleMemPool(const SimpleMemPool &);
  SimpleMemPool& operator==(const SimpleMemPool &);

private:
  void FreeAll();
  void* head_;
  LockType locker_;
};

template<int size, class LockType>
void* SimpleMemPool<size, LockType>::Allocate() {
  if (!head_) return std::malloc(size);

  ScopedMutex<LockType> locker(locker_);
  if (!head_) return std::malloc(size);

  void* p = head_;
  head_ = *static_cast<void**>(p);
  return p;
}

template<int size, class LockType>
void SimpleMemPool<size, LockType>Release(void* p) {
  ScopedMutex<LockType> locker(locker_);
  void** ptr = static_cast<void**>(p);
  *ptr = head_;
  head_ = p;
}

tempalte<int size, class LockType>
void SimpleMemPool<size, LockType>::FreeAll() {
  ScopedMutex<LockType> locker(locker_);
  while (head_) {
    void* p = head_;
    head_ = *static_cast<void**>(p);
    std::free(p);
  }//end-while.
}

}//end-cromwell.

#endif
