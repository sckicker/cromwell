#ifndef __CROMWELL_THREAD_H
#define __CROMWELL_THREAD_H

#include <stdexcept>
#include <stdlib.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sched.h>

namespace cromwell {

typedef std::function<void(void*)> ThreadFunc;

class Thread {
public:
  enum ThreadState {
    kStop = 0,
    kRunning,
  };

  explicit Thread(const ThreadFunc& func, pthread_attr* attr, bool detached) :
    func_(func),
    attr_(attr),
    detached_(detached),
    state_(kStop) {

    }

  ~Thread() {
    if (attr_) {
      pthread_attr_destroy(attr_);
      delete attr_;
      attr_ = nullptr;
    }
    if (!detached_) {
      try { this->Join(); } catch(...) {}
    }
  }

  inline bool Start(void* arg) {
    if (state_ != kStop) return false;
    if (pthread_create(&t_id_, attr_, DefaultThreadMain, arg) != 0) {
      throw std::runtime_error("pthread_create failed.");
    }
    state_ = kRunning;
    return true;
  }

  inline bool Join() {
    if (!detached_ && state_ != kStop) {
      void* ignore;
      int res = pthread_join(t_id_, &ignore);
      detached_ = (res == 0);
      if (res != 0) return false;
    }//end-if.
    return true;
  }

  static inline bool IsCurrent(pthread_t id) {
    return pthread_equal(pthread_self(), id);
  }

  static inline pthread_t GetCurrent() {
    return pthread_self();
  }

  static inline void SetName(const char* thread_name) {
    prctl(PR_SET_NAME, name);
  }

  static inline void Yield() {
    sched_yield();
  }

  inline pthread_t Id() const { return t_id_; }
  inline bool Running() const { return state_ == kRunning; }

private:
  inline static void* DefaultThreadMain(void* arg) {
    func_(arg);
    return NULL;
  }

private:
  ThreadFunc func_;
  pthread_attr_t* attr_;
  pthread_t t_id_;
  ThreadState state_;
  bool detached_;
};

class ThreadFactory {
public:
  ThreadFactory()
    : priority_(-1),
    schedule_(-1),
    stack_size_(-1),
    detached_(false) {

    }

  inline void set_priority(float prior) {
    if (prior >= 1) priority_ = 1.0f;
    else if(prior <= 0) priority_ = 0.0f;
    else priority_ = prior;
  }

  inline int get_priority(void) const {
    int policy = get_schedule();
  }

  inline void set_schedule(int policy) {
    switch(policy) {
      case SCHED_FIFO:
      case SCHED_RR:
      case SCHED_OTHER:
        schedule_ = policy;
        break;
      default:
        schedule_ = SCHED_OTHER;
        break;
    }//end-switch.
  }

  inline int get_schedule(void) const {
    return schedule_;
  }

  inline void set_stack_size(int mbytes) {
    stack_size_ = mbytes;
  }

  inline int get_stack_size() const {
    return stack_size_;
  }

  inline void set_detached(bool rc) {
    detached_ = rc;
  }

  inline bool get_detached() const {
    return detached_;
  }

  inline Thread* CreateThread(const ThreadFunc& func) const {
    pthread_attr_t* attr = new pthread_attr_t;
    if (0 == pthread_attr_init(attr)) {
      do {
        int k = detached_ ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE;
        if (pthread_attr_setdetachstate(attr, k) != 0) break;

        if (stack_size_ > 0 && pthread_attr_setstacksize(attr, 1024*1024*stack_size_) != 0)
          break;

        if (schedule_ >= 0 && pthread_attr_setschedpolicy(attr, schedule_) != 0) break;

        if (priority_ >= 0) {
          struct sched_param param;
          param.sched_priority = get_priority();
          if (pthread_attr_setschedparam(attr, &sched_param) != 0) break;
        }
        return new Thread(func, attr, detached_);
      } while(0);
    }//end-if.
    delete attr;
    return nullptr;
  }

private:
  float priority_;
  int schedule_;
  int stack_size_;
  bool detached_;
};

}//end-cromwell.

#endif
