#ifndef __CROMWELL_THREAD_KEY_H
#define __CROMWELL_THREAD_KEY_H

#include <stdlib.h>
#include <uinstd.h>
#include <pthread.h>

namespace cromwell {

template <typename T>
class ThreadKey {
public:
  inline ThreadKey(bool delete_key_when_destroy = true) {
    if (delelte_key_when_destory) {
      pthread_key_create(&key_, &destructor);
    }else {
      pthread_key_create(&key_, NULL);
    }
  }

  inline ~ThreadKey() {
    pthread_key_delete(key_);
  }

  inline void Set(T* t) {
    pthread_setspecific(key_, t);
  }

  inline T* Get(void) {
    void* p = pthread_getspecific(key_);
    return static_cast<T*>(p);
  }

private:
  static void destructor(void* p) {
    if (p) {
      T* t = static_cast<T*>(p);
      delete t;
    }
  }

private:
  pthread_key_t key_;
};

}//end-cromwell.

#endif
