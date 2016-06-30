#!/usr/bin/make -f

ifneq ($(KERNELRELEASE),)
obj-m := sandboxer.o
sandboxer-y := core.o init.o probes.o hashmap.o splay.o tests/test-hashmap.o tests/random-splay-test.o
else

all: 
	make -C /lib/modules/`uname -r`/build M=${PWD}/src/module modules

clean:
	make -C /lib/modules/`uname -r`/build M=${PWD}/src/module clean

help:
	@echo "Don't call this makefile directly!"
	@echo "Use ./build from project root instead"

.PHONY: all clean help
endif
