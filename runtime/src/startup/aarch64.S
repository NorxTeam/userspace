.section .text._start,"ax"
.global _start
.type _start,%function
_start:
    mov x0, sp
    bl norx_start
    mov w8, #60
    svc #0
1:
    wfe
    b 1b
.size _start, .-_start
