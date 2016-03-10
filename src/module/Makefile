obj-m += sandboxer.o

all: 
	make -C /lib/modules/${shell uname -r}/build M=${PWD}/src/module modules

clean:
	make -C /lib/modules/${shell uname -r}/build M=${PWD}/src/module clean
	
