obj-m := rha-keypad.o
rha-keypad-objs:= keypad.o

export CROSS_COMPILE:= /opt/Embedsky/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/tq-linaro-toolchain/bin/arm-none-linux-gnueabi-
CC :=$(CROSS_COMPILE)gcc
KDIR := /opt/work/ArgentinaPIS/linux_E9_3.0.35_for_Linux
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

app:
	$(CC) -o getkey getkey.c		
clean:
	rm -rf *.o  *.cmd *.ko *.mod.c modules.order  Module.symvers 
	rm -rf .*.cmd  .tmp_versions
	rm -rf getkey



