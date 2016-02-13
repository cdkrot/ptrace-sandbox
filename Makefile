#!/usr/bin/make -f

CFLAGS=-Wall -Wextra -std=gnu11 -ggdb3
CPPFLAGS=-Wall -Wextra -std=gnu++11 -ggdb3

all: detect_syscalls aplusb return-1-cpp return-minus-1-c return-0-noglibc brk-1-noglibc newdetect hello

detect_syscalls: src/detect_syscalls/detect_syscalls.c src/detect_syscalls/tracing_utils.h src/detect_syscalls/tracing_utils.c
	gcc ${CFLAGS} src/detect_syscalls/detect_syscalls.c src/detect_syscalls/tracing_utils.c -o detect_syscalls

hello: src/tests/hello.c
	gcc ${CFLAGS} src/tests/hello.c -o hello

aplusb: src/tests/aplusb.cpp
	g++ ${CPPFLAGS} src/tests/aplusb.cpp -o aplusb

return-1-cpp: src/tests/return-1.cpp
	g++ ${CPPFLAGS} src/tests/return-1.cpp -o return-1-cpp

return-0-noglibc: src/tests/return-0-noglibc.s
	gcc -nostdlib src/tests/return-0-noglibc.s -o return-0-noglibc

brk-1-noglibc: src/tests/brk-1-noglibc.s
	gcc -nostdlib src/tests/brk-1-noglibc.s -o brk-1-noglibc

return-minus-1-c: src/tests/return-minus-1.c
	gcc ${CFLAGS} src/tests/return-minus-1.c -o return-minus-1-c

newdetect: src/newdetect.c
	gcc ${CFLAGS} src/newdetect.c -rdynamic -o newdetect

clean:
	rm -f aplusb
	rm -f detect_syscalls
	rm -f return-1
	rm -f return-1-cpp
	rm -f return-0-noglibc
	rm -f brk-1-noglibc
	rm -f return-minus-1-c
	rm -f newdetect
	rm -f hello
	rm -f res/aplusb.out

.PHONY:   all clean
.DEFAULT: all
