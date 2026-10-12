#include "mem_utils.h"
#include <string.h>

void secure_zero_memory(void* ptr, size_t size) {
    if (!ptr || size == 0) return;
#if defined(__STDC_LIB_EXT1__) || defined(HAVE_EXPLICIT_BZERO)
    explicit_bzero(ptr, size);
#else
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    while (size--) {
        *p++ = 0;
    }
#endif
}

int fast_memcmp_c(const void* s1, const void* s2, size_t n) {
    if (s1 == s2 || n == 0) return 0;
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;

    // Check if pointers are 8-byte aligned for word comparison
    if ((((uintptr_t)p1 | (uintptr_t)p2) & 7) == 0) {
        const uint64_t* u1 = (const uint64_t*)p1;
        const uint64_t* u2 = (const uint64_t*)p2;
        while (n >= 8) {
            if (*u1 != *u2) {
                p1 = (const unsigned char*)u1;
                p2 = (const unsigned char*)u2;
                break;
            }
            u1++;
            u2++;
            n -= 8;
        }
        if (n >= 8) {
            // Found difference in the 8-byte block
            for (size_t i = 0; i < 8; ++i) {
                if (p1[i] != p2[i]) {
                    return (int)p1[i] - (int)p2[i];
                }
            }
        }
        p1 = (const unsigned char*)u1;
        p2 = (const unsigned char*)u2;
    }

    // Remaining tail bytes
    while (n--) {
        if (*p1 != *p2) {
            return (int)*p1 - (int)*p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

int64_t fast_strscan_c(const char* s, size_t len, char needle) {
    if (!s || len == 0) return -1;
    for (size_t i = 0; i < len; ++i) {
        if (s[i] == needle) {
            return (int64_t)i;
        }
    }
    return -1;
}

uint64_t fast_fnv1a64_c(const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    const uint64_t fnv_prime = 1099511628211ULL;

    for (size_t i = 0; i < len; ++i) {
        hash ^= (uint64_t)p[i];
        hash *= fnv_prime;
    }
    return hash;
}

int64_t fast_sum_paise_c(const int64_t* amounts, size_t count) {
    if (!amounts || count == 0) return 0;
    int64_t sum = 0;
    size_t i = 0;

    // Unroll by 4
    for (; i + 3 < count; i += 4) {
        sum += amounts[i] + amounts[i + 1] + amounts[i + 2] + amounts[i + 3];
    }
    for (; i < count; ++i) {
        sum += amounts[i];
    }
    return sum;
}
