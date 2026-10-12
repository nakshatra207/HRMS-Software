#ifndef HRMS_LOWLEVEL_MEM_UTILS_H
#define HRMS_LOWLEVEL_MEM_UTILS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Securely zeroes out sensitive memory (passwords, salts, keys)
 * in a way that will never be optimized away by the compiler.
 */
void secure_zero_memory(void* ptr, size_t size);

/**
 * High-performance 64-bit word-aligned memory comparison in pure C.
 */
int fast_memcmp_c(const void* s1, const void* s2, size_t n);

/**
 * High-performance byte scanning in pure C.
 * Returns the 0-based index of the first occurrence of needle, or -1 if not found.
 */
int64_t fast_strscan_c(const char* s, size_t len, char needle);

/**
 * 64-bit FNV-1a hash algorithm in pure C.
 */
uint64_t fast_fnv1a64_c(const void* data, size_t len);

/**
 * Sums an array of 64-bit integers (e.g. monetary paise) in pure C.
 */
int64_t fast_sum_paise_c(const int64_t* amounts, size_t count);

#ifdef __cplusplus
}
#endif

#endif // HRMS_LOWLEVEL_MEM_UTILS_H
