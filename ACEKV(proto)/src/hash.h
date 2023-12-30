#ifndef HASH_H
#define    HASH_H
#include <cstdint>
#include <cstddef>
#ifdef    __cplusplus
extern "C" {
#endif

uint32_t jenkins_hash(const void *key, size_t length, const uint32_t initval);

#ifdef    __cplusplus
}
#endif

#endif    /* HASH_H */

