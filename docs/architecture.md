# System Architecture & Component Design

This document details the architectural principles, layer decomposition, and module interactions of the Lightweight Human Resource Management System (HRMS).

---

## 1. Architectural Philosophy

The application is engineered around four core tenets:
1. **Zero Bloat & Native Performance**: Avoid heavyweight runtimes (Node.js, Electron, JVM, Python). Run as a standalone ELF binary compiled with native toolchains.
2. **Minimal Resource Utilization**: Target <10 MB RAM baseline footprint and sub-millisecond cold start times, allowing smooth operation even on legacy or constrained hardware.
3. **Defense in Depth**: Zero reliance on client-side validation; 100% parameterized SQLite prepared statements; strict Role-Based Access Control; constant-time cryptographic verification; secure memory sanitization.
4. **Honest Engineering**: Use Assembly strictly where it makes sense, enforce clean C-ABI wrappers, and guarantee 100% functional parity with pure C/C++ fallback routines.

---

## 2. Layered Architecture

```
+-------------------------------------------------------------------------+
|                  Presentation Layer: Terminal UI (TUI)                  |
|     - VT100 / ANSI Colored Interface                                    |
|     - Interactive Navigation, Form Prompts & Tabular Reports            |
|     - Headless Batch CLI Mode (--benchmark, --export-report)            |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
|                       Application Business Services                     |
|  - EmployeeService     - AttendanceService     - LeaveService           |
|  - PayrollService      - RecruitmentService    - DepartmentService      |
|  - ReportService       - AuthService           - AuditLogger            |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
|                       Data Access Layer (SQLite RAII)                   |
|  - Database (Connection, WAL Mode, Pragmas, Schema Migrations)          |
|  - Statement (Prepared Statement Lifecycle, Parameter Binding)          |
|  - Transaction (Atomic Commit/Rollback, Autocommit State Awareness)     |
+-------------------------------------------------------------------------+
                  |                                     |
                  v                                     v
+-----------------------------------+ +-----------------------------------+
|      Low-Level C11 Primitives     | |       x86-64 NASM Assembly        |
|  - mem_utils: secure zeroing      | |  - asm_fast_memcmp: 64-bit cmp    |
|  - posix_sys: monotonic clock,    | |  - asm_fast_strscan: byte search  |
|    process memory (/proc/statm)   | |  - asm_fnv1a64: 64-bit hash       |
|  - sha256: FIPS PUB 180-4 cipher  | |  - asm_sum_paise: 64-bit sum      |
+-----------------------------------+ +-----------------------------------+
```

---

## 3. Technology Partitioning

### C++20 Layer
* **Scope**: Domain business logic, database wrappers, transactional workflows, reporting, formatting, and UI.
* **Modern Features Utilized**:
  * **RAII Lifecycle**: Automatic database statement finalization, automatic transaction rollback on exceptions, safe file handles.
  * `std::string_view` & `std::span`: Eliminates heap copies when parsing inputs, comparing identifiers, or processing arrays of numbers.
  * `std::optional`: Explicit representation of present or absent database records without null pointer bugs.
  * **Move Semantics**: Vector return values transfer ownership in $O(1)$ time without copying.

### C11 Layer
* **Scope**: Low-level system integration, cryptographic hashing, and OS process metrics.
* **Components**:
  * `c/lowlevel/mem_utils.c`: Memory zeroing resistant to compiler dead-store elimination; baseline C word-aligned memory comparisons.
  * `c/lowlevel/posix_sys.c`: Reads `/proc/self/statm` for precise resident memory consumption; queries `clock_gettime(CLOCK_MONOTONIC)` for nanosecond benchmarking.
  * `c/lowlevel/sha256.c`: Clean, portable, high-speed SHA-256 implementation with zero external OpenSSL dependencies.

### x86-64 NASM Assembly Layer
* **Scope**: Hot computational kernels where 64-bit register operations yield measurable performance.
* **ABI Compliance**: Complies with the **System V AMD64 ABI**:
  * Arguments passed in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`.
  * Return values passed in `rax`.
  * Non-executable stack marked via `.note.GNU-stack` section.
* **Pure C/C++ Fallback**: Every Assembly routine has an identical pure C fallback in `mem_utils.c`. If NASM is absent or `enable_assembly=false` is set in configuration, the system automatically falls back without disruption.

---

## 4. Memory Management Strategy

1. **Stack Over Heap**: All small buffers, timestamps, and parameters reside on the call stack.
2. **Deterministic Ownership**: Owning pointers are managed via `std::unique_ptr`. Raw pointers are strictly non-owning references.
3. **No Hidden Background Threads**: The system does not run idle polling loops or background threads that eat CPU cycles. All operations are strictly event-driven and request-driven.
