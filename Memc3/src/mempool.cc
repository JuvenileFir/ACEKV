#include "mempool.h"


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
