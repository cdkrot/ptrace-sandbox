#!/usr/bin/make -f

# < Configuration >

  CFLAGS=-Wall -Wextra -std=gnu11   -Isrc/utils -Isrc/gen -ggdb3
CPPFLAGS=-Wall -Wextra -std=gnu++11 -Isrc/utils -Isrc/gen -ggdb3

# < default target >
all:   generated demos main utils

# < Main targets, form: deps, key targets >

demos:     utils  hello aplusb return-1-cpp return-minus-1-c return-0-noglibc brk-1-noglibc
main:      utils  newdetect
generated: utils  src/gen/
utils: # nothing at the moment


clean:
	rm -f hello aplusb return-1-cpp return-minus-1-c return-0-noglibc brk-1-noglibc
	rm -f newdetect
	rm -f res/aplusb.out
	rm -Rf src/gen/

.PHONY:   all demos main generated utils clean
.DEFAULT: all

# < Do-nothing targets >
src/util/naming_utils.h:
scripts/gen-naming-utils-c.py:

# < Demos >

hello: src/demos/hello.c
	gcc ${CFLAGS} src/demos/hello.c -o hello

aplusb: src/demos/aplusb.cpp
	g++ ${CPPFLAGS} src/demos/aplusb.cpp -o aplusb

return-1-cpp: src/demos/return-1.cpp
	g++ ${CPPFLAGS} src/demos/return-1.cpp -o return-1-cpp

return-minus-1-c: src/demos/return-minus-1.c
	gcc ${CFLAGS} src/demos/return-minus-1.c -o return-minus-1-c

return-0-noglibc: src/demos/return-0-noglibc.s
	gcc -nostdlib src/demos/return-0-noglibc.s -o return-0-noglibc

brk-1-noglibc: src/demos/brk-1-noglibc.s
	gcc -nostdlib src/demos/brk-1-noglibc.s -o brk-1-noglibc

# < Mkdir targets >

src/gen/:
	mkdir -p src/gen/

# < Main targets >

newdetect: src/main/newdetect.c src/gen/naming_utils.c src/util/naming_utils.h
	gcc ${CFLAGS} src/main/newdetect.c src/gen/naming_utils.c -Isrc/utils -rdynamic -o newdetect

# < Utils >

src/gen/naming_utils.c: scripts/gen-naming-utils-c.py src/gen/
	python3 scripts/gen-naming-utils-c.py < res/syscalls.list > src/gen/naming_utils.c
