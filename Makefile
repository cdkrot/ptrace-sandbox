#!/usr/bin/make -f

CFLAGS=-Wall -Wextra -std=gnu11 -Isrc/include -ggdb3
CPPFLAGS=-Wall -Wextra -std=gnu++11 -Isrc/include -ggdb3

all: detect_syscalls aplusb return-1-cpp return-minus-1-c return-0-noglibc brk-1-noglibc newdetect hello

detect_syscalls: src/legacy/detect_syscalls.c src/legacy/tracing_utils.h src/legacy/tracing_utils.c
	gcc ${CFLAGS} src/legacy/detect_syscalls.c src/legacy/tracing_utils.c -o detect_syscalls

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

newdetect: obj/ obj/newdetect.o obj/associative_array.o
	gcc obj/newdetect.o obj/associative_array.o -rdynamic -o newdetect

obj/:
	mkdir -p obj/

obj/associative_array.o: src/associative_array.c src/include/associative_array.h
	gcc ${CFLAGS} src/associative_array.c -c -o obj/associative_array.o

obj/newdetect.o: src/newdetect.c
	gcc ${CFLAGS} src/newdetect.c -c -o obj/newdetect.o

clean:
	rm -f aplusb
	rm -f detect_syscalls
	rm -f return-1-cpp
	rm -f return-0-noglibc
	rm -f brk-1-noglibc
	rm -f return-minus-1-c
	rm -f newdetect
	rm -f hello
	rm -f res/aplusb.out
	rm -Rf obj

.PHONY:   all clean
.DEFAULT: all
