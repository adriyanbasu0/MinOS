[BITS 64]
section .text
global enable_cpu_features

enable_cpu_features:
    mov rax, cr0
    and rax, ~(1 << 2)          ; Clear EM bit (bit 2)
    or  rax,  (1 << 1)          ; Set MP bit (bit 1)
    mov cr0, rax

    mov rax, cr4
    or  rax, (1 << 9) | (1 << 10)  ; Set OSFXSR (bit 9) and OSXMMEXCPT (bit 10)
    mov cr4, rax

    ret
