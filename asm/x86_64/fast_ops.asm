; ==============================================================================
; Lightweight HRMS - x86-64 NASM Assembly Optimizations
; System V AMD64 ABI Compliant
; ==============================================================================

section .text

; -----------------------------------------------------------------------------
; int asm_fast_memcmp(const void* rdi, const void* rsi, size_t rdx)
; Returns 0 if identical, <0 if *s1 < *s2, >0 if *s1 > *s2
; -----------------------------------------------------------------------------
global asm_fast_memcmp
asm_fast_memcmp:
    test rdx, rdx
    jz .equal
    cmp rdi, rsi
    je .equal

.loop_qword:
    cmp rdx, 8
    jb .loop_byte
    mov rax, [rdi]
    cmp rax, [rsi]
    jne .diff_in_qword
    add rdi, 8
    add rsi, 8
    sub rdx, 8
    jmp .loop_qword

.diff_in_qword:
    ; A difference exists in the 8-byte block. Find exact byte difference.
    mov rdx, 8

.loop_byte:
    test rdx, rdx
    jz .equal
    movzx eax, byte [rdi]
    movzx ecx, byte [rsi]
    cmp eax, ecx
    jne .byte_diff
    inc rdi
    inc rsi
    dec rdx
    jmp .loop_byte

.byte_diff:
    sub eax, ecx
    ret

.equal:
    xor eax, eax
    ret

; -----------------------------------------------------------------------------
; int64_t asm_fast_strscan(const char* rdi, size_t rsi, char dl)
; Scans buffer for needle byte. Returns 0-based index or -1 if not found.
; -----------------------------------------------------------------------------
global asm_fast_strscan
asm_fast_strscan:
    test rsi, rsi
    jz .not_found

    mov r8, rdi             ; Save base pointer
    movzx edx, dl           ; Needle byte

.scan_loop:
    cmp byte [rdi], dl
    je .found
    inc rdi
    dec rsi
    jnz .scan_loop

.not_found:
    mov rax, -1
    ret

.found:
    mov rax, rdi
    sub rax, r8             ; index = current_ptr - base_ptr
    ret

; -----------------------------------------------------------------------------
; uint64_t asm_fnv1a64(const void* rdi, size_t rsi)
; High-performance 64-bit FNV-1a Hash for fast hash index lookups
; -----------------------------------------------------------------------------
global asm_fnv1a64
asm_fnv1a64:
    mov rax, 0xcbf29ce484222325   ; FNV offset basis
    mov r8,  0x100000001b3        ; FNV prime

    test rsi, rsi
    jz .done

.fnv_loop:
    movzx rcx, byte [rdi]
    xor rax, rcx
    imul rax, r8
    inc rdi
    dec rsi
    jnz .fnv_loop

.done:
    ret

; -----------------------------------------------------------------------------
; int64_t asm_sum_paise(const int64_t* rdi, size_t rsi)
; Unrolled 64-bit integer summation for batch payroll calculations
; -----------------------------------------------------------------------------
global asm_sum_paise
asm_sum_paise:
    xor rax, rax            ; sum = 0
    test rsi, rsi
    jz .sum_done

.sum_loop4:
    cmp rsi, 4
    jb .sum_tail
    add rax, [rdi]
    add rax, [rdi + 8]
    add rax, [rdi + 16]
    add rax, [rdi + 24]
    add rdi, 32
    sub rsi, 4
    jmp .sum_loop4

.sum_tail:
    test rsi, rsi
    jz .sum_done
    add rax, [rdi]
    add rdi, 8
    dec rsi
    jmp .sum_tail

.sum_done:
    ret

; Mark non-executable stack for GNU security compliance
section .note.GNU-stack noalloc noexec nowrite progbits
