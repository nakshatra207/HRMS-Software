# Developer & Contributor Guide

This document details guidelines for compiling, extending, testing, and maintaining the Lightweight HRMS codebase.

---

## 1. Build Configurations

The project supports three primary CMake build modes:

### Portable Release (Default)
Recommended for distribution packages across heterogeneous x86-64 Linux systems:
```bash
./build.sh release
# Flags: -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wpedantic
```

### Local Optimized Release
Targets the local host CPU with microarchitecture instruction set extensions (AVX2, BMI2):
```bash
./build.sh local
# Flags: -O3 -march=native -Wall -Wextra -Wpedantic
```

### Debug Build
Compiles with full debugging symbols and zero optimizations for GDB / Valgrind profiling:
```bash
./build.sh debug
# Flags: -O0 -g -Wall -Wextra -Wpedantic
```

---

## 2. Directory Layout & Conventions

When introducing new features or modules:
* Header interfaces belong in `include/<module_name>/<module>.hpp`.
* C++ implementations belong in `src/<module_name>/<module>.cpp`.
* Low-level C primitives belong in `c/lowlevel/`.
* Assembly routines belong in `asm/x86_64/`.
* Database migrations belong in `database/`.

### Coding Standards
* **C++ Standard**: C++20. Use `std::string_view` for read-only string parameters and `std::span` for array buffers.
* **RAII**: Always wrap external handles (`sqlite3*`, `sqlite3_stmt*`, `FILE*`, file descriptors) in RAII classes.
* **Namespaces**: Place all code inside the `hrms::` root namespace and module sub-namespaces (e.g., `hrms::emp`, `hrms::att`, `hrms::pay`, `hrms::leave`, `hrms::sec`, `hrms::utils`).
* **Header Guards**: Use `#pragma once` across all C++ headers and standard `#ifndef` guards in C headers.
* **Exceptions**: Use `hrms::db::DatabaseException` or domain result types; never silently swallow database or file errors.

---

## 3. Running the Test Suite

Before committing any modifications, run the automated test suite:

```bash
./build.sh test
```

The test runner will execute all 11 test suites and assert correct results across transactions, rollbacks, monetary math, and security filters.

### Memory Leak Profiling with Valgrind (Optional)
```bash
valgrind --leak-check=full --show-leak-kinds=all ./build/hrms_test
```

---

## 4. Adding a New Assembly Routine

1. Add the assembly function to `asm/x86_64/fast_ops.asm` using standard System V AMD64 ABI:
   * First 6 arguments: `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`.
   * Return register: `rax`.
   * Include `.note.GNU-stack` at the bottom of the assembly file.
2. Implement an identical pure C fallback in `c/lowlevel/mem_utils.c`.
3. Expose the C++ wrapper in `include/utilities/fast_ops.hpp` with runtime fallback dispatching.
4. Add verification assertions in `tests/test_hrms.cpp`.
