#ifndef __CROMWELL_ALLOCATOR_H
#define __CROMWELL_ALLOCATOR_H

#include <type_traits>
#include <new>
#include <cassert>

#include "simple_mempool.h"

namespace cromwell {

namespace detail {
  template <typename T, class trivial>
  class ReleaseTrait {
  public:
    static void release(T* pointer) {}
  };

  template <typename T>
  class ReleaseTrait<T, std::false_type> {
  public:
    static void release(T* pointer) { pointer->~T(); }
  }
}//end-namespace-detail.

template <typename T, class LockType=MutexType>
class PoolAllocator {
public:
  typedef SimpleMemPool<sizeof(T), LockType> ThisPool;

  static T* alloc() {
    void* p = ThisPool::Instance().Allocate();
    if (!p) return nullptr;
    return new(p) T();
  }

  static T* alloc(const T& obj) {
    void* p = ThisPool::Instance().Allocate();
    if (!p) return nullptr;
    return new(p) T(obj);
  }

  static void release(T* pointer) {
    typedef typename std::has_trivial_destructor<T>::type M;
    detail::ReleaseTrait<T, M>::release(pointer);
    ThisPool::Instance().Release(pointer);
  }

  static void* malloc(size_t size) {
    assert(size == sizeof(T));
    return ThisPool::Instance().Allocate();
  }

  static void* free(void* pointer) {
    ThisPool::Instance().Release(pointer);
  }

private:
  PoolAllocator(const PoolAllocator &);
  PoolAllocator& operator=(const PoolAllocator &);
};

template <typename T>
class NewAllocator {
public:
  static T* alloc() {
    return new T();
  }

  static T* alloc(const T& obj) {
    return new T(obj);
  }

  static void release(T* pointer) {
    delete pointer;
  }

  static void* malloc(size_t size) {
    return new T();
  }

  static void free(void* pointer) {
    delete static_cast<T*>(pointer);
  }

private:
  NewAllocator(const NewAllocator &);
  NewAllocator& operator=(const NewAllocator &);
};

#define CLASS_ALLOCATOR_DECL_BASE(ALLOCATOR, T) \
  void Destroy() { \
    ALLOCATOR<T>::release(this); \
  } \
  static T* Allocate() {
    return ALLOCATOR<T>::alloc(); \
  } \
  static T* Allocate() { \
    return ALLOCATOR<T>::alloc(other); \
  } \

#ifdef USE_MEM_POOL
#define CLASS_ALLOCATOR_DECL(T) \
  CLASS_ALLOCATOR_DECL_BASE(PoolAllocator, T)
#else
#define CLASS_ALLOCATOR_DECL(T) \
  CLASS_ALLOCATOR_DECL_BASE(NewAllocator, T)
#endif

}//end-namespace-cromwell.

#endif
