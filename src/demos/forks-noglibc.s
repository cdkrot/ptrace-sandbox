.globl _start

.data
msg:    
    .ascii "Hello, world\n"
    len = . - msg
	
.text
_start:
    # SYS_clone
    movq $56, %rax
    movq $0,  %rdi
    movq $0,  %rsi
    movq $0,  %rdx
    movq $0,  %r10
    movq $0,  %r8
    syscall

    # SYS_write
    movq $1, %rax
    movq $1,   %rdi
    movq $msg, %rsi
    movq $len, %rdx
    syscall

    # SYS_exit
	movq $60, %rax
	movq $3, %rdi
    syscall
