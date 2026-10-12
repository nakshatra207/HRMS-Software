#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <span>

namespace hrms::utils {

/**
 * Global configuration to enable or disable Assembly optimization at runtime.
 */
void set_assembly_enabled(bool enabled);
bool is_assembly_active();
bool is_assembly_compiled();

/**
 * Fast memory comparison.
 * Returns 0 if identical, <0 if buf1 < buf2, >0 if buf1 > buf2.
 */
int fast_memcmp(const void* buf1, const void* buf2, size_t size);

/**
 * Fast byte search.
 * Returns 0-based offset or -1 if not found.
 */
int64_t fast_strscan(std::string_view str, char needle);

/**
 * 64-bit FNV-1a hash calculation for fast string hashing and lookups.
 */
uint64_t fast_hash64(std::string_view data);
uint64_t fast_hash64(const void* data, size_t len);

/**
 * Fast fixed-point summation for arrays of monetary values (paise).
 */
int64_t fast_sum_paise(std::span<const int64_t> amounts);
int64_t fast_sum_paise(const int64_t* amounts, size_t count);

} // namespace hrms::utils
