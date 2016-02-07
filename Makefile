#!/usr/bin/make -f

CFLAGS=-Wall -Wextra -std=c11
CPPFLAGS=-Wall -Wextra -std=c++11

all: detect_syscalls aplusb return-1

detect_syscalls: src/detect_syscalls.c
	gcc ${CFLAGS} src/detect_syscalls.c -o detect_syscalls

aplusb: src/aplusb.cpp
	g++ ${CPPFLAGS} src/aplusb.cpp -o aplusb

return-1: src/return-1.cpp
	g++ ${CPPFLAGS} src/return-1.cpp -o return-1

clean:
	rm -f aplusb
	rm -f detect_syscalls
	rm -f return-1
	rm -f res/aplusb.out

.PHONY:   all clean
.DEFAULT: all
