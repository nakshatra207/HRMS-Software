#define _POSIX_C_SOURCE 200809L
#include "posix_sys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/resource.h>

uint64_t get_monotonic_time_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

int get_process_memory_usage(size_t* rss_kb, size_t* vms_kb) {
    if (!rss_kb || !vms_kb) return -1;
    *rss_kb = 0;
    *vms_kb = 0;

    FILE* fp = fopen("/proc/self/statm", "r");
    if (fp) {
        unsigned long size_pages = 0;
        unsigned long resident_pages = 0;
        if (fscanf(fp, "%lu %lu", &size_pages, &resident_pages) == 2) {
            long page_size = sysconf(_SC_PAGESIZE);
            if (page_size > 0) {
                *vms_kb = (size_pages * (size_t)page_size) / 1024;
                *rss_kb = (resident_pages * (size_t)page_size) / 1024;
                fclose(fp);
                return 0;
            }
        }
        fclose(fp);
    }

    // Fallback to getrusage
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        *rss_kb = (size_t)usage.ru_maxrss; // In Linux, ru_maxrss is in KiB
        *vms_kb = *rss_kb;
        return 0;
    }

    return -1;
}

int get_cpu_count(void) {
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    return (nprocs > 0) ? (int)nprocs : 1;
}

void terminal_get_size(int* rows, int* cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        if (rows) *rows = ws.ws_row;
        if (cols) *cols = ws.ws_col;
    } else {
        if (rows) *rows = 24;
        if (cols) *cols = 80;
    }
}

void get_current_datetime_str(char* buf, size_t max_len) {
    if (!buf || max_len == 0) return;
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(buf, max_len, "%Y-%m-%d %H:%M:%S", &tm_info);
}

void get_current_date_str(char* buf, size_t max_len) {
    if (!buf || max_len == 0) return;
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(buf, max_len, "%Y-%m-%d", &tm_info);
}
