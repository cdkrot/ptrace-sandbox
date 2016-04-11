//  Demos to test sandboxing stuff on
//  Copyright (C) 2016  Sayutin Dmitry, Vasiliy Alferov
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.                                                                                                                      

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
