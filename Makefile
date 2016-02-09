#!/usr/bin/make -f

CFLAGS=-Wall -Wextra -std=c11
CPPFLAGS=-Wall -Wextra -std=c++11

all: detect_syscalls aplusb return-1-cpp return-minus-1-c return-0-noglibc

detect_syscalls: src/detect_syscalls.c src/tracing_utils.h src/tracing_utils.c
	gcc ${CFLAGS} src/detect_syscalls.c src/tracing_utils.c -o detect_syscalls

aplusb: src/aplusb.cpp
	g++ ${CPPFLAGS} src/aplusb.cpp -o aplusb

return-1-cpp: src/return-1.cpp
	g++ ${CPPFLAGS} src/return-1.cpp -o return-1-cpp

return-0-noglibc: src/return-0-noglibc.s
	gcc -nostdlib src/return-0-noglibc.s -o return-0-noglibc

return-minus-1-c: src/return-minus-1.c
	gcc ${CFLAGS} src/return-minus-1.c -o return-minus-1-c

clean:
	rm -f aplusb
	rm -f detect_syscalls
	rm -f return-1
	rm -f return-1-cpp
	rm -f return-0-noglibc
	rm -f return-minus-1-c
	rm -f res/aplusb.out

.PHONY:   all clean
.DEFAULT: all
