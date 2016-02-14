#!/usr/bin/make -f

# < Configuration >

CFLAGS=-Wall -Wextra -std=gnu11 -Isrc/include -ggdb3
CPPFLAGS=-Wall -Wextra -std=gnu++11 -Isrc/include -ggdb3

# < Main targets >
all: newdetect demos

demos: hello aplusb return-1-cpp return-minus-1-c return-0-noglibc brk-1-noglibc

clean:
	rm -f hello aplusb return-1-cpp return-minus-1-c return-0-noglibc brk-1-noglibc
	rm -f newdetect
	rm -f res/aplusb.out
	rm -Rf obj
	rm -Rf gen

.PHONY:   all demos clean
.DEFAULT: all

# < Demos >

hello: src/tests/hello.c
	gcc ${CFLAGS} src/tests/hello.c -o hello

aplusb: src/tests/aplusb.cpp
	g++ ${CPPFLAGS} src/tests/aplusb.cpp -o aplusb

return-1-cpp: src/tests/return-1.cpp
	g++ ${CPPFLAGS} src/tests/return-1.cpp -o return-1-cpp

return-minus-1-c: src/tests/return-minus-1.c
	gcc ${CFLAGS} src/tests/return-minus-1.c -o return-minus-1-c

return-0-noglibc: src/tests/return-0-noglibc.s
	gcc -nostdlib src/tests/return-0-noglibc.s -o return-0-noglibc

brk-1-noglibc: src/tests/brk-1-noglibc.s
	gcc -nostdlib src/tests/brk-1-noglibc.s -o brk-1-noglibc

# < Mkdir targets >

obj/:
	mkdir -p obj/

src/gen/:
	mkdir -p src/gen

# < New detect >

newdetect: obj/ obj/newdetect.o obj/associative_array.o
	gcc obj/newdetect.o obj/associative_array.o -rdynamic -o newdetect

obj/newdetect.o: src/newdetect.c
	gcc ${CFLAGS} src/newdetect.c -c -o obj/newdetect.o

# < Utils >

obj/associative_array.o: src/associative_array.c src/include/associative_array.h
	gcc ${CFLAGS} src/associative_array.c -c -o obj/associative_array.o
