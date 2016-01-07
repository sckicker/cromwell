#ifndef __CROMWELL_FIXED_MEMPOOL_H
#define __CROMWELL_FIXED_MEMPOOL_H

#include <stdlib.h>
#include <stdint.h>
#include <new>

#include "mutex.h"

namespace cromwell {

template <class AllocLock=MutexType, class FreeLock=MutexType>
class FixedSizeMemPool {
public:
  FixedSizeMemPool();
  ~FixedSizeMemPool();

  bool Initialize(uint32_t block_size, uint32_t block_count);
  void Destory(void);

  uint32_t GetBlockSize() const;
  uint32_t GetBlockCount() const;
  uint32_t GetUsedCount() const;
  uint32_t GetFreeCount() const;

  void* Alloc();
  bool Free(void* block);

  uint64_t GetKey(void* block) const;
  void* GetBlock(uint64_t key) const;

public:
  static uint32_t GetId(uint64_t key) {
    BlockHeader* bh = (BlockHeader*)&key;
    return hb->id;
  }
  static uint64_t TotalSize(uint32_t block_size, uint32_t block_count) {
    return sizeof(GlobalHeader) + (sizeof(uint32_t) + sizeof(BlockHeader) + block_size) * block_count;
  }

protected:
  #pragma pack(push, 1)
  struct GlobalHeader {
    uint32_t block_size;
    uint32_t block_count;
    uint32_t begin;
    uint32_t end;
    uint32_t next_magic;
    uint32_t ids[1];
  };

  struct BlockHeader {
    union {
      uint64_t key;
      struct {
        uint32_t id;
        uint32_t magic;
      }
    };
    uint8_t data[0];
  };
  #pragma pack(pop)

  inline BlockHeader* get_block_header(uint32_t idx) const;

  inline bool check_id(BlockHeader* header) const;

  inline static BlockHeader* block_header(void* data) {
    uint8_t* p = (uint8_t*)data;
    p -= sizeof(BlockHeader);
    return (BlockHeader*)p;
  }

private:
  FixedSizeMemPool(const FixedSizeMemPool &);
  FixedSizeMemPool& operator==(const FixedSizeMemPool &);
private:
  GlobalHeader* mem_;
  AllocLock alloc_locker_;
  FreeLock free_locker_;
};


template <class T, class AlockLock=MutexType, class FreeLock=MutexType>
class FixedSizeAllocator {
public:
  bool Initialze(uint32_t count) {
    return mem_pool_.Initialze(sizeof(T), count);
  }

  inline uint32_t Capacity() const {
    return mem_pool_.GetBlockCount();
  }

  inline uint32_t UsedCount() const {
    return mem_pool_.GetUsedCount();
  }

  inline uint32_t GetKey(T* block) const {
    return mem_pool_.GetKey(block);
  }

  inline T* GetBlock(uint64_t key) const {
    return (T*)(mem_pool_.GetBlock(key));
  }

  inline bool Check(T* block) const {
    return mem_pool_.GetKey(block) > 0;
  }

  inline bool Release(T* obj) {
    obj->~T();
    return mem_pool_.Free(obj);
  }

  inline T* Allocate() {
    void* obj = mem_pool_.Alloc();
    if (obj) return new(obj) T();
    return nullptr;
  }

  template <class P>
  inline T* Allocate(const P& p) {
    void* obj = mem_pool_.Alloc();
    if (obj) return new(obj) T(p);
    return nullptr;
  }
  
private:
  FixedSizeMemPool<AllocLock, FreeLock> mem_pool_;
};

}//end-cromwell.
