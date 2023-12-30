#include "piekv.h"
#include "assoc_cuckoo.h"

struct timeval t0, t_latency[4], t_end[4], t_init;   

// struct timeval t0, t_init;   
extern unsigned int hash_items;
extern unsigned long int hashsize;

MemPool *kMemPool;
bool allow_mutation = false;
Piekv::Piekv(size_t init_table_size, size_t init_log_size){
    is_running_ = 1;
    max_tput_ = 0;
    for (int i = 0; i < THREAD_NUM; i++) {
      thread_is_running_[i] = 1;
      latency[i] = 0;
    }
    /******************
    *
    * 类型:记录
    * 内容:stop_entry_gc_项根据原注释的意思是用于在H2L是停止垃圾收集？
    *      该项继承自FlexibleKV，PieKV中只有初始化。暂时保留该项。
    * 添加者:bwb
    * 时间:2023-02-24
    *
    ******************/

    // kMemPool = new MemPool(init_block_size, init_mem_block_number);  // TODO: move this line to main function later, it shouldn't be here
    kMemPool = new MemPool(init_table_size, init_log_size);  
    // log_ = new Log(mempool_, init_log_block_number);
    all_table_stats_ = new AllTableStats();
    memset(all_table_stats_,0,sizeof(AllTableStats));

}

Piekv::~Piekv() {
    printf("deleting piekv......\n");
}


void Piekv::print_trigger(size_t maxbytes) {
    // printf(" == [STAT] printer started on CORE 36 == \n");
    // bind memflowing thread to core 34
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(36, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
        fprintf(stderr, "[E] set thread affinity failed\n");
    }
    bool do_print = false;
    uint64_t res[12][2], cuckoonum = 0;
    memset(res, 0, sizeof(res));
    gettimeofday(&t_init, NULL);
    AllTableStats* stat = all_table_stats_;
    double time_init = t_init.tv_sec + t_init.tv_usec / 1000000.0, time[2]= {0.0, 0.0}, timediff = 0.;
    while (is_running_) {
        gettimeofday(&t0, NULL);
        time[0] = (t0.tv_sec + t0.tv_usec / 1000000.0);
        timediff = time[0] - time[1];
        
        res[0][0] = stat->count;
        res[2][0] = stat->set_success;
        res[3][0] = stat->rx_packet;
        res[4][0] = stat->rx_burst;
        res[1][0] = stat->get_found;
        res[6][0] = stat->get_request;
        res[7][0] = stat->set_request;
        res[5][0] = stat->set_request + stat->get_request;

        if (res[0][0] != res[0][1] || res[1][0] != res[1][1] || res[3][0] != res[3][1] ||
            res[2][0] != res[2][1] || res[4][0] != res[4][1] || res[5][0] != res[5][1] || res[6][0] != res[6][1]) {
              do_print = true;
              printf("\n[INFO]Current Time:       %.2lf s\n",time[0] - time_init);   
              printf("\n[P]indexs Load Factor:    %.4lf %%\n", 100.0 * hash_items / bkt_size / hashsize);
              // printf("\n[P]indexs Load Factor:    %.4lf %%\n", stat->count * 100.0 / kSlotNum );
              printf("\n[P]slabs Load Factor:     %.4lf %%\n", stat->count * 64 * 100.0 / maxbytes);
              printf("\n[P]EVICT Ratio:           %.4lf %%\n", stat->set_evict_succ * 64 * 100.0 / maxbytes);
              printf("\n[P]set request:           %zu \n", res[7][0]/* stat->set_request */);
              printf("\n[P]get request:           %zu \n", res[6][0]/* stat->get_request */);
              printf("\n[P]Get Hit Rate:          %.4lf %%\n", (res[1][0] - res[1][1]) * 100.0 / (res[6][0] - res[6][1]));
              printf("\n[P]ACC Hit Rate:          %.4lf %%\n", res[1][0] * 100.0 / res[6][0]);            
              printf("\n[P]total cuckoo num:      %zu \n", cuckoo_count);
              printf("[P]total cuckoo path:     %zu \n", cuckoo_depth);

            }
        if (res[3][0]!=res[3][1]) printf("\n[P]rx packet:             %7.3lf / s\n",
                                          (res[3][0] - res[3][1]) / timediff );
        if (res[5][0]!=res[5][1]) {
          printf("\n[P]Throughput:            %7.3lf ops\n", (res[5][0] - res[5][1]) / timediff );
          printf("\n[P]all request:           %lu o\n", res[5][0] );
          max_tput_ = std::max(max_tput_, (res[5][0] - res[5][1]) / timediff);
        }
        if (res[0][0]!=res[0][1]) printf("\n[P]Valid Entries:         %7.3lf / s\n",
                                          (res[0][0] - res[0][1]) / timediff );
        // if (res[4][0]!=res[4][1])  printf("\n[P]rx burst:        %7.3lf M / s\n",
                                          // (res[4][0] - res[4][1]) / timediff /1000000. );
        if (res[2][0]!=res[2][1]) printf("\n[P]set success:           %7.3lf / s\n",
                                          (res[2][0] - res[2][1]) / timediff );
        if (res[1][0]!=res[1][1]) printf("\n[P]get found:             %7.3lf / s\n",
                                          (res[1][0] - res[1][1]) / timediff );
        if (do_print) printf("\n----------------------------------\n");

        for (int i = 0; i < 12; i++)  {
          res[i][1] = res[i][0];
          res[i][0] = 0;
        }
        time[1] = time[0];
        do_print = false;
        sleep(1);
        // usleep(600000);
        for (int i = 0; i < 4; i++)  {
          latency[i] = 0;        
        }
    }
}