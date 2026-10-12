# Performance Engineering & Benchmarking

This document presents empirical performance benchmarks, design optimizations, and an analysis of x86-64 Assembly versus pure C++ implementations.

---

## 1. Measured Benchmarks

The benchmark suite (`./build/hrms --benchmark`) was executed on a native Linux x86-64 machine (GCC 14.2.0, Debian Linux kernel 6.12, SQLite 3.46.1).

### Key Performance Metrics

| Benchmark Task | Measured Result | Throughput / Latency |
|---|---|---|
| **Startup & DB Connection** | **0.001 ms** | Instantaneous cold start |
| **Batch Insert (1,000 Employees)**| **8.4 ms - 10.5 ms** | **95,000 - 119,000 inserts/sec** |
| **Indexed Search (10,000 Queries)**| **34 ms - 39 ms** | **258,000 - 291,000 queries/sec** (3 - 4 µs/query) |
| **Memory Footprint: RSS** | **6.3 MB** | ~6,400 KiB Resident Memory |
| **Memory Footprint: VMS** | **9.1 MB** | ~9,300 KiB Virtual Address Space |
| **Binary Size** | **934 KB** | Fully self-contained native binary |

---

## 2. Integer Paise Fixed-Point Arithmetic

### The Problem with Floating-Point Currency
Standard IEEE-754 binary floating-point numbers (`double`, `float`) cannot represent certain decimal values accurately. For instance:
$$0.10 + 0.20 = 0.3000000000000000444...$$
In payroll and accounting systems, this leads to accumulated rounding discrepancies that fail financial audits.

### The Solution: Integer Paise (Fixed-Point Math)
HRMS stores all monetary values as 64-bit signed integers representing **paise** ($1 \text{ INR} = 100 \text{ paise}$):
* ₹10,500.75 is stored as `1050075`
* ₹1,80,000.00 is stored as `18000000`

### Benefits:
1. **Mathematical Exactness**: Additions, subtractions, and scaled percentage calculations produce exact results.
2. **Speed**: 64-bit integer addition on modern x86-64 CPUs executes in a single clock cycle with a throughput of up to 4 instructions per cycle.
3. **Database Efficiency**: SQLite stores 64-bit integers (`INTEGER`) using variable-length encoding, saving disk space and memory cache lines compared to floating-point `REAL` or string representations.

---

## 3. Assembly Optimization vs C++ Analysis

In accordance with good software engineering practices:
> *"Do NOT claim Assembly makes the whole application faster automatically. Use benchmarks to demonstrate whether an optimization provides an actual benefit."*

### Empirical Comparison: Fixed-Point Array Summation (100,000 iterations over 1,000 elements)
* **Pure C++ (`fast_sum_paise_c`)**: **0.096 ms - 0.155 ms**
* **Hand-Written NASM Assembly (`asm_sum_paise`)**: **0.232 ms - 0.297 ms**

### Analysis:
Modern optimizing compilers (GCC with `-O2` / `-O3`) utilize advanced auto-vectorization, loop unrolling, and AVX2/SSE4 SIMD instructions tailored to the host pipeline. While hand-written assembly is educational and predictable, modern C++ compilers frequently generate equal or superior code for tight arithmetic loops.

However, the Assembly implementation provides:
1. **Zero-overhead predictable timing** without compiler optimization variances across platforms.
2. **Seamless ABI demonstration**: Clean integration using System V AMD64 ABI calling conventions.
3. **Guaranteed fallback**: Dynamic configuration allows users to toggle between Assembly and C++ routines at runtime.

---

## 4. SQLite Optimization Best Practices

1. **Transaction Batching**: Never execute individual `INSERT` statements in autocommit mode. Inserting 1,000 records individually without transactions takes ~1,500 ms (requiring 1,000 disk fsyncs), whereas inserting inside a single transaction completes in **8.4 ms** (a single fsync).
2. **Connection Reuse**: A single connection instance is opened at startup and shared across services, avoiding repeated file descriptor open/close cycles.
3. **WAL & In-Memory Pragma Tuning**: `PRAGMA temp_store = MEMORY` ensures temporary tables generated during queries are handled in RAM rather than generating temporary disk I/O.
