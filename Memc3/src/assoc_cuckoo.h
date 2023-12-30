#pragma once
#ifndef ASSOC_CUCKOO_H_
#define ASSOC_CUCKOO_H_

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <xmmintrin.h>
#include <immintrin.h>

#include "memcached.h"
#include "memc3_config.h"
#include "memc3_util.h"
#include "bit_util.h"

extern uint64_t cuckoo_depth;
extern uint64_t cuckoo_count;
/* associative array */
void assoc2_init(/* const  */int hashpower_init);
item *assoc2_find(const char *key, const size_t nkey, const uint32_t hv);
int assoc2_insert(item *item, const uint32_t hv);
void assoc2_delete(const char *key, const size_t nkey, const uint32_t hv);
/* void do_assoc_move_next_bucket(void); */
/* int start_assoc_maintenance_thread(void); */
/* void stop_assoc_maintenance_thread(void); */

void assoc2_destroy(void);
void assoc2_pre_bench(void);
void assoc2_post_bench(void);

void assoc2_print();
#endif