.globl _start
	
.text
_start:
    movq $12, %rax
    movq $1,  %rdi
    syscall

	movq $1, %rax
	xorq %rbx, %rbx # return value
	int $0x80
