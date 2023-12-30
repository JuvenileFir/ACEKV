#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdint.h>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <algorithm>

#ifdef __cplusplus
#define EXTERN_BEGIN extern "C" {
#define EXTERN_END }
#else
#define EXTERN_BEGIN
#define EXTERN_END
#endif

typedef uint32_t Cbool;
#define true ((Cbool)1)
#define false ((Cbool)0)

#define ROUNDDOWN40(x) (((x) / 40) * 40 )//针对8+8的KVsize特别定制

#define ROUNDUP8(x) (((x) + 7UL) & (~7UL))
#define ROUNDUP64(x) (((x) + 63UL) & (~63UL))
#define ROUNDUP4K(x) (((x) + 4095UL) & (~4095UL))
#define ROUNDUP1M(x) (((x) + 1048575UL) & (~1048575UL))
#define ROUNDUP2M(x) (((x) + 2097151UL) & (~2097151UL))

#define ALWAYS_INLINE __attribute__((always_inline))
#define UNUSED __attribute__((unused))
#define ALIGNED(alignment) __attribute__((aligned(alignment)))

#define PARTITION_SIZE_IN_BYTE (size_t)(2 * 1024 * 1024)  // 2MB Pages, hard Coded.
#define ITEMS_PER_BUCKET ((uint32_t)7)                    // the first 64 bits is used for metadata of a bucket
#define ITEMS_PER_BUCKET_7
// ToThink: Why `- 1` can improve load factor? But `- 1` can speedup operation by switch `%` to `&`.
#define BUCKETS_PER_PARTITION (16384UL * 2 - 1)  // 0x7fff. So we can optimize this by using & operator.

#define EXPIRED ((uint32_t)0x87654321)
#define VALID ((uint32_t)0x12345678)
#define BATCH_RESERVED ((uint32_t)2)
#define OFFSET_SPACE ((uint64_t)0x7ffffffffff)

#ifdef COLLECT_STATS
#define TABLE_STAT_INC(table, name)                 \
  do {                                              \
    __sync_add_and_fetch(&(table)->tstats.name, 1); \
  } while (0)
#define TABLE_STAT_DEC(table, name)                 \
  do {                                              \
    __sync_sub_and_fetch(&(table)->tstats.name, 1); \
  } while (0)
#define STORE_STAT_ADD(store, name, size)              \
  do {                                                 \
    __sync_add_and_fetch(&(store)->sstats.name, size); \
  } while (0)
#define STORE_STAT_SUB(store, name, size)              \
  do {                                                 \
    __sync_sub_and_fetch(&(store)->sstats.name, size); \
  } while (0)
#else
#define TABLE_STAT_INC(table, name) \
  do {                              \
    (void)table;                    \
  } while (0)
#define TABLE_STAT_DEC(table, name) \
  do {                              \
    (void)table;                    \
  } while (0)
#define STORE_STAT_ADD(store, name, size) \
  do {                                    \
    (void)store;                          \
  } while (0)
#define STORE_STAT_SUB(store, name, size) \
  do {                                    \
    (void)store;                          \
  } while (0)
#endif

#define PRINT_EXCECUTION_TIME(msg, code, ...)            \
  do {                                                   \
    struct timeval t1, t2;                               \
    double elapsed;                                      \
    gettimeofday(&t1, NULL);                             \
    do {                                                 \
      code;                                              \
    } while (0);                                         \
    gettimeofday(&t2, NULL);                             \
    elapsed = (t2.tv_sec - t1.tv_sec) * 1000.0;          \
    elapsed += (t2.tv_usec - t1.tv_usec) / 1000.0;       \
    printf(msg " [time: %f ms]\n", __VA_ARGS__ elapsed); \
  } while (0);

EXTERN_BEGIN


typedef enum PiekvStatus {
  SUCCESS_SET,
  BATCH_FULL,
  BATCH_TOO_SMALL,
  FAILURE_ALREADY_EXIST,
  FAILURE_HASHTABLE_FULL,
} PiekvStatus;

static inline void memory_barrier() { asm volatile("" ::: "memory"); }

static inline size_t next_power_of_two(size_t v) {
  size_t s = 0;
  while (((size_t)1 << s) < v) s++;
  return (size_t)1 << s;
}


static inline size_t shm_adjust_size(size_t size) { return (uint64_t)ROUNDUP2M(size); }

static inline Cbool memcmp8(const uint8_t *dest, const uint8_t *src, size_t length) {
  length = ROUNDUP8(length);
  switch (length >> 3) {
    case 0:
      return true;
    case 1:
      if (*(const uint64_t *)(dest + 0) != *(const uint64_t *)(src + 0)) return false;
      return true;
    case 2:
      if (*(const uint64_t *)(dest + 0) != *(const uint64_t *)(src + 0)) return false;
      if (*(const uint64_t *)(dest + 8) != *(const uint64_t *)(src + 8)) return false;
      return true;
    case 3:
      if (*(const uint64_t *)(dest + 0) != *(const uint64_t *)(src + 0)) return false;
      if (*(const uint64_t *)(dest + 8) != *(const uint64_t *)(src + 8)) return false;
      if (*(const uint64_t *)(dest + 16) != *(const uint64_t *)(src + 16)) return false;
      return true;
    case 4:
      if (*(const uint64_t *)(dest + 0) != *(const uint64_t *)(src + 0)) return false;
      if (*(const uint64_t *)(dest + 8) != *(const uint64_t *)(src + 8)) return false;
      if (*(const uint64_t *)(dest + 16) != *(const uint64_t *)(src + 16)) return false;
      if (*(const uint64_t *)(dest + 24) != *(const uint64_t *)(src + 24)) return false;
      return true;
    default:
      return memcmp(dest, src, length) == 0;
  }
}

static inline void memcpy8(uint8_t *dest, const uint8_t *src, size_t length) {
  length = ROUNDUP8(length);
  switch (length >> 3) {
    case 0:
      break;
    case 1:
      *(uint64_t *)(dest + 0) = *(const uint64_t *)(src + 0);
      break;
    case 2:
      *(uint64_t *)(dest + 0) = *(const uint64_t *)(src + 0);
      *(uint64_t *)(dest + 8) = *(const uint64_t *)(src + 8);
      break;
    case 3:
      *(uint64_t *)(dest + 0) = *(const uint64_t *)(src + 0);
      *(uint64_t *)(dest + 8) = *(const uint64_t *)(src + 8);
      *(uint64_t *)(dest + 16) = *(const uint64_t *)(src + 16);
      break;
    case 4:
      *(uint64_t *)(dest + 0) = *(const uint64_t *)(src + 0);
      *(uint64_t *)(dest + 8) = *(const uint64_t *)(src + 8);
      *(uint64_t *)(dest + 16) = *(const uint64_t *)(src + 16);
      *(uint64_t *)(dest + 24) = *(const uint64_t *)(src + 24);
      break;
    default:
      memcpy(dest, src, length);
      break;
  }
}

static inline uint32_t mehcached_rand(uint64_t *state) {
  // same as Java's
  *state = (*state * 0x5deece66dUL + 0xbUL) & ((1UL << 48) - 1);
  return (uint32_t)(*state >> (48 - 32));
}

static inline double mehcached_rand_d(uint64_t *state) {
  // caution: this is maybe too non-random
  *state = (*state * 0x5deece66dUL + 0xbUL) & ((1UL << 48) - 1);
  return (double)*state / (double)((1UL << 48) - 1);
}



EXTERN_END
