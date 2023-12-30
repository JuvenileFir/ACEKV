#include "mempool.h"

/* static int mem_blocks_compare_vaddr(const void *a, const void *b) {
  const struct MemBlock *pa = (const struct MemBlock *)a;
  const struct MemBlock *pb = (const struct MemBlock *)b;
  if (pa->ptr < pb->ptr)
    return -1;
  else
    return 1;
} */

/* MemPool::MemPool(size_t block_size, size_t block_num_to_init) {
    block_size_ = block_size;
    printf("MEM: Initializing pages...\n");

    for (size_t i = 0; i < block_num_to_init; i++) {
        // void *ptr = mmap(NULL, block_size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        void *ptr = malloc(block_size_);
        if (ptr == MAP_FAILED) {
            printf("MEM: map failed.\n");
        }
        mem_blocks_[i].ptr = ptr;
        if (mem_blocks_[i].ptr == NULL) break;   // TODO: what does this mean? and why is it here?
        memset(mem_blocks_[i].ptr, 0, block_size_);   // TODO: is this really necessary?
        blocknum_++;
    }
    blocknum_in_use_ = 0;
    printf("MEM:   initial allocation of %u blocks\n", blocknum_);
    printf("MEM:   sorting by virtual address\n");
    
    // ******************
    // *
    // * 类型:解释
    // * 内容:PieKV分配内存后将对memblock数组按分配的虚拟地址进行重新排序，使得每个block空间呈虚拟空间有序排列。
    // * 添加者:bwb
    // * 时间:2023-02-24
    // *
    // ******************
    qsort(mem_blocks_, blocknum_, sizeof(MemBlock), mem_blocks_compare_vaddr);
} */
//new MemPool()
MemPool::MemPool(size_t table_size, size_t log_size) {
    printf("MEM: Initializing pages...\n");
    /******************
    *
    * 类型:新添加内容说明
    * 内容：在这里通过把新分配空间赋值给了MemPool的两个成员指针变量，来实现table和log两大模块的分配。需要注意的是mica和PieKV最
    *      初的设计是利用hugepage来分配内存空间，而此时ProtoKV未使用，因此throughout性能可能会弱于使用hugepage的实例
    * 添加者:bwb
    * 时间:2023-02-24
    *
    ******************/
    for (int i = 0; i < THREAD_NUM; i++) {
        max_log_size_ = log_size;
        // printf("max_log_size_: %ld\n", max_log_size_);
        log_ptr_[i] = malloc(max_log_size_);
        if (log_ptr_[i] == MAP_FAILED) printf("MEM: log map failed.\n");
    }
    
    max_table_size_ = table_size;
    table_ptr_ = malloc(max_table_size_);
    if (table_ptr_ == MAP_FAILED) printf("MEM: table map failed.\n");
}

MemPool::~MemPool() {

}

LogItem *MemPool::locate_item(const uint16_t seg, uint64_t offset) {
    return reinterpret_cast<LogItem*>(log_ptr_[seg] + (offset & (max_log_size_ - 1)));
}
/* void MemPool::free_all_blocks() {
    for (uint32_t i = 0; i < blocknum_; i++){
        free(mem_blocks_[i].ptr);
    }
    blocknum_ = 0;
    blocknum_in_use_ = 0;
} */

/*  
    find a free block in mempool and return its id
    return -1 to sign full
 */
/* uint32_t MemPool::alloc_block() {
    // Q: linear search may be low-efficient here?
    for (uint32_t i = 0; i < blocknum_; i++)
    {
        if (mem_blocks_[i].in_use == 0) {
            mem_blocks_[i].in_use = 1;
            blocknum_in_use_++;
            return i;
        }
    }
    return -1;
} */

// get block ptr by block number
/* void *MemPool::get_block_ptr(uint32_t blockNumber) {
    return mem_blocks_[blockNumber].ptr;
} */

/* size_t MemPool::get_block_size() {
    return block_size_;
} */

// clean the last `num_pages` pages of circular_log before being used by hash table. 
/* void MemPool::memset_block(uint32_t blockNumber) {
    memset(get_block_ptr(blockNumber), 0, block_size_);
} */

/* uint32_t MemPool::get_block_available_num() {
    return (blocknum_ - blocknum_in_use_);
} */

/* LogItem *MemPool::locate_item(const uint32_t blockNumber, uint64_t logOffset) {
    return (LogItem *)((uint8_t*)get_block_ptr(blockNumber) + logOffset);
} */