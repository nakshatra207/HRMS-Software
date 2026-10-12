#ifndef HRMS_LOWLEVEL_POSIX_SYS_H
#define HRMS_LOWLEVEL_POSIX_SYS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns monotonic time in nanoseconds for micro-benchmarks and high-precision timing.
 */
uint64_t get_monotonic_time_ns(void);

/**
 * Queries current process memory metrics in Kilobytes.
 * Reads /proc/self/statm and page size on Linux.
 * Returns 0 on success, -1 on failure.
 */
int get_process_memory_usage(size_t* rss_kb, size_t* vms_kb);

/**
 * Returns the number of online CPU cores.
 */
int get_cpu_count(void);

/**
 * Queries the terminal window size (rows and columns).
 * Falls back to 24x80 if stdout is not a TTY or ioctl fails.
 */
void terminal_get_size(int* rows, int* cols);

/**
 * Formats current UTC/local timestamp as ISO-8601 string: YYYY-MM-DD HH:MM:SS
 */
void get_current_datetime_str(char* buf, size_t max_len);

/**
 * Formats current date as YYYY-MM-DD
 */
void get_current_date_str(char* buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // HRMS_LOWLEVEL_POSIX_SYS_H
