#KERNEL_SRC must be define

#one module multiple objects
#module-objs := readmap.o

#obj-m := module.o 
obj-m := queue.o 

ccflags-$(DEBUG) += -DDEBUG

#multiple modules
#obj-m := module.o readmap.o

#CC=gcc

#MODCFLAGS := -Wall -DMODULE -D__KERNEL__ -DLINUX -I$(KERNEL_SRC)/include

#$(KERNEL_SRC)/include/linux/version.h
#module.o: module.c 
#	$(CC) $(MODCFLAGS) -c module.c

#all : module.o

DIR=$CD
options :=

ifdef CONFIG
 options += KCONFIG_CONFIG=${CONFIG}
endif

v := uname -r
# build for the current host
host : module

module:
	echo `uname -r`
	make -C /usr/src/linux-headers-`uname -r` M=$(CURDIR) ${options}  modules
	
clean:
	rm -f *.ko *.mod.* *.order *.o