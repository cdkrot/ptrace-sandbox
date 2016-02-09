.globl _start
	
.text
_start:
	movl $1, %eax
	movl $0, %ebx # return value
	int $0x80
