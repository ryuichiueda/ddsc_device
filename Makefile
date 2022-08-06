#Reference: http://www.devdrv.co.jp/linux/kernel26-makefile.htm
TARGET:= ddsc.ko

all: ${TARGET}

ddsc.ko: ddsc.c
	make -C /usr/src/linux-headers-`uname -r` M=`pwd` V=1 modules

clean:
	make -C /usr/src/linux-headers-`uname -r` M=`pwd` V=1 clean

obj-m:= ddsc.o

clean-files := *.o *.ko *.mod.[co] *~
