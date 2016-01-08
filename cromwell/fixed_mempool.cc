#include "fixed_mempool.h"

namespace cromwell {

FixedSizeMemPool::FixedSizeMemPool()
: mem_(nullptr) {

}

FixedSizeMemPool::~FixedSizeMemPool() {
  Destory();
}

void FixedSizeMemPool::Destory() {
  if (mem_) {
    ::free(mem_);
    mem_ = nullptr;
  }
}

bool FixedSizeMemPool::Init(uint32_t block_size, uint32_t block_count) {
  uint64_t size = FixedSizeMemPool::TotalSize(block_size, block_count);
  void* p = mem_ ? ::realloc(mem_, size) : ::malloc(size);
  if (!p) return false;
  mem_  = (GlobalHeader*)p;
  mem_->block_size = sizeof(BlockHeader) + block_size;
  mem_->block_count = block_count;
  mem_->begin = 0;
  mem_->end = block_count;
  mem_->next_magic = 0;

  for (uint32_t i = 0; i < block_count; ++i) {
    mem_->ids[i] = i;
    get_block_header(i)->key = 0;
  }//end-for.

  return true;
}

void* FixedSizeMemPool::Alloc() {
  uint32_t id, magic;
  bool okay = false;

  {
    ScopedMutex<AllocLock> locker(alloc_locker_);
    uint32_t b = mem_->begin;
    if (b != mem_->end) {
      okay = true;
      id = mem_->ids[b];
      b = (b == mem_->block_count ? 0 : b+1);
      mem_->begin = b;
      magic = mem_->next_magic + 1;
      if (magic == 0) magic = 1;
      mem_->next_magic = magic;
    }//end-if.
  }

  if (okay) {
    BlockHeader* header = get_block_header(id);
    header->magic = magic;
    header->id = id;
    return header->data;
  }
  return nullptr;
}

bool FixedSizeMemPool::Free(void* block) {
  BlockHeader* bh = block_header(block);
  if (check_id(bh) && bh->magic != 0) {
    uint32_t id = bh->id;
    bh->key = 0;
    {
      ScopedMutex<FreeLock> locker(free_locker_);
      uint32_t e = mem_->end;
      mem_->ids[e] = id;
      e = (e == mem_->block_count ? 0 : e+1);
      mem_->end = e;
    }
    return true;
  }//end-if
  return false;
}

uint32_t FixedSizeMemPool::GetBlockSize() const {
  return mem_->block_size - sizeof(BlockHeader);
}

uint32_t FixedSizeMemPool::GetBlockCount() const {
  return mem_->block_count;
}

uint32_t FixedSizeMemPool::GetUsedCount() const {
  int32_t n = mem_->begin - mem_->end;
  return (n > 0) ? (n-1) : (mem_->block_count + n);
}

uint32_t FixedSizeMemPool::GetFreeCount() const {
  int32_t n = mem_->end - mem_->begin;
  return (n >= 0) ? n : (mem_->block_count + n + 1);
}

uint64_t GetKey(void* block) const {
  BlockHeader* bh = block_header(block);
  if (check_id(hb) && header->magic != 0) return header->key;
  return 0;
}

void* GetBlock(uint64_t key) const {
  BlockHeader* hb = (BlockHeader*)&key;
  if (hb->magic == 0) return nullptr;
  if (hb->id >= mem_->block_count) return nullptr;
  BlockHeader* header = get_block_header(hb->id);
  return (header->key == key ? header->data : nullptr);
}

FixedSizeMemPool::BlockHeader* FixedSizeMemPool::get_block_header(uint32_t idx) const {
  uint32_t offset = sizeof(FixedSizeMemPool::GlobalHeader);
  offset += sizeof(uint32_t) * mem_->block_count;
  uint8_t* p = (uint8_t*)mem_;
  p += offset + (idx * mem_->block_size);
  return (BlockHeader*)p;
}

bool FixedSizeMemPool::check_id(BlockHeader* header) const {
  return (header->id < mem_->block_count) && (header == get_block_header(header->id));
}

}//end-cromwell.
