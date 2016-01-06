#ifndef __CROMWELL_FIXED_FIFO_H
#define __CROMWELL_FIXED_FIFO_H

#include "mutex.h"
#include "sema.h"

namespace cromwell {

template <class T, class ReadingMutex=MutexType, class WritingMutex=MutexType>
class FifoQueue {
public:
  explicit FifoQueue(size_t capacity)
  : capacity_(capacity),
  sema_writing_(capacity, capacity),
  sema_reading_(capacity, 0) {
    container_ = new T[capacity+1];
    id_writing_ = id_reading_ = 0;
  }

  ~FifoQueue() {
    delete[] container_;
    container_ = NULL;
  }

  bool PushBack(const T& v) {
    if (sema_writing_.TryWait()) {
      {
        ScopedMutex<WritingMutex> locker(mutex_writing_);
        uint32_t w = id_writing_;
        container_[w] = v;
        id_writing_ = (w == capacity_ ? 0 : w+1);
      }
      sema_reading_.Post();
      return true;
    }//end-if
    return false;
  }

  bool PopFront(T* v) {
    if (sema_reading_.TryWait()) {
      {
        ScopedMutex<ReadingMutex> locker(mutex_reading_);
        uint32_t r = id_reading_;
        *v = container_[r];
        id_reading_ = (r == capacity_ ? 0 : r+1);
      }
      sema_writing_.Post();
      return true;
    }//end-if
    return false;
  }

  bool PopFront(double sec, T* v) {
    if (sema_reading_.Wait(sec)) {
      {
        ScopedMutex<ReadingMutex> locker(mutex_reading_);
        uint32_t r = id_reading_;
        *v = container_[r];
        id_reading_ = (r == capacity_ ? 0 : r+1);
      }
      sema_writing_.Post();
      return true;
    }//end-if
    return false;
  }

  inline uint32_t UsedSize() const {
    uint32_t delta = id_writing - id_reading_;
    return delta >= 0 ? delta : (capacity_ + 1 + delta);
  }

  inline size_t Capacity() const {
    return capacity_;
  }

  bool PushFront(const T& v) {
    if (sema_writing_.TryWait()) {
      {
        ScopedMutex<ReadingMutex> locker(mutex_reading_);
        uint32_t r = id_reading_;
        id_reading_ = (r == 0 ? capacity_ : r-1);
        container_[id_reading_] = v;
      }
      sema_reading_.Post();
      return true;
    }//end-if
    return false;
  }

private:
  T* container_;
  size_t capacity_;

  uint32_t id_writing_;
  char padding_1[64-sizeof(uint32_t)];
  uint32_t id_reading_;
  char padding_2[64-sizeof(uint32_t)];

private:
  WritingMutex mutex_writing_;
  ReadingMutex mutex_reading_;

  SemaType sema_writing_;
  SemaType sema_reading_;

  FifoQueue(const FifoQueue &);
  FifoQueue& operator=(const FifoQueue &);
};

}//end-cromwell
