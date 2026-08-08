.section .text._start,"ax",@progbits
.global _start
.type _start,@function
_start:
    movq %rsp, %rdi
    callq norx_start
    movl %eax, %edi
    movl $60, %eax
    syscall
1:
    hlt
    jmp 1b
.size _start, .-_start
