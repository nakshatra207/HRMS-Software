#include "utilities/fast_ops.hpp"
#include <atomic>

extern "C" {
#include "mem_utils.h"

#ifdef HRMS_HAS_ASM
extern int asm_fast_memcmp(const void* s1, const void* s2, size_t n);
extern int64_t asm_fast_strscan(const char* s, size_t len, char needle);
extern uint64_t asm_fnv1a64(const void* data, size_t len);
extern int64_t asm_sum_paise(const int64_t* amounts, size_t count);
#endif
}

namespace hrms::utils {

static std::atomic<bool> g_assembly_enabled{true};

void set_assembly_enabled(bool enabled) {
    g_assembly_enabled.store(enabled, std::memory_order_relaxed);
}

bool is_assembly_active() {
#ifdef HRMS_HAS_ASM
    return g_assembly_enabled.load(std::memory_order_relaxed);
#else
    return false;
#endif
}

bool is_assembly_compiled() {
#ifdef HRMS_HAS_ASM
    return true;
#else
    return false;
#endif
}

int fast_memcmp(const void* buf1, const void* buf2, size_t size) {
#ifdef HRMS_HAS_ASM
    if (g_assembly_enabled.load(std::memory_order_relaxed)) {
        return asm_fast_memcmp(buf1, buf2, size);
    }
#endif
    return fast_memcmp_c(buf1, buf2, size);
}

int64_t fast_strscan(std::string_view str, char needle) {
#ifdef HRMS_HAS_ASM
    if (g_assembly_enabled.load(std::memory_order_relaxed)) {
        return asm_fast_strscan(str.data(), str.size(), needle);
    }
#endif
    return fast_strscan_c(str.data(), str.size(), needle);
}

uint64_t fast_hash64(std::string_view data) {
    return fast_hash64(data.data(), data.size());
}

uint64_t fast_hash64(const void* data, size_t len) {
#ifdef HRMS_HAS_ASM
    if (g_assembly_enabled.load(std::memory_order_relaxed)) {
        return asm_fnv1a64(data, len);
    }
#endif
    return fast_fnv1a64_c(data, len);
}

int64_t fast_sum_paise(std::span<const int64_t> amounts) {
    return fast_sum_paise(amounts.data(), amounts.size());
}

int64_t fast_sum_paise(const int64_t* amounts, size_t count) {
#ifdef HRMS_HAS_ASM
    if (g_assembly_enabled.load(std::memory_order_relaxed)) {
        return asm_sum_paise(amounts, count);
    }
#endif
    return fast_sum_paise_c(amounts, count);
}

} // namespace hrms::utils
