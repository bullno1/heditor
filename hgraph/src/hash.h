#ifndef HGRAPH_HASH_H
#define HGRAPH_HASH_H

#include <stdint.h>

#ifdef _MSC_VER

#include <intrin.h>

static inline uint32_t
hash_ilog2(uint32_t x) {
	uint32_t result;
	_BitScanReverse(&result, x);
	return result;
}

#else

#include <limits.h>

static inline uint32_t
hash_ilog2(uint32_t x) {
    return sizeof(uint32_t) * CHAR_BIT - __builtin_clz(x) - 1;
}

#endif

static inline int32_t
hash_exp(int32_t size) {
	return (int32_t)hash_ilog2((uint32_t)size) + 1;
}

static inline int32_t
hash_size(int32_t exp) {
	return 1 << exp;
}

// https://nullprogram.com/blog/2022/08/08/
static inline int32_t
hash_msi(uint64_t hash, int32_t exp, int32_t idx) {
    uint32_t mask = ((uint32_t)1 << exp) - 1;
    uint32_t step = (hash >> (64 - exp)) | 1;
    return (idx + step) & mask;
}

// https://lemire.me/blog/2018/08/15/fast-strongly-universal-64-bit-hashing-everywhere/
static inline uint64_t
hash_murmur64(uint64_t h) {
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdL;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53L;
	h ^= h >> 33;
	return h;
}

#endif
