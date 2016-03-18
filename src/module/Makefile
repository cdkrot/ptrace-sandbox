#!/usr/bin/make -f

ifneq ($(KERNELRELEASE),)
obj-m := sandboxer.o
sandboxer-y := sandboxer-core.o sandboxer-proc.o
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
