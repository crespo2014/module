#KERNEL_SRC must be define

#one module multiple objects
#module-objs := readmap.o

#obj-m := module.o 
obj-m := queue.o 

#multiple modules
#obj-m := module.o readmap.o

#CC=gcc

#MODCFLAGS := -Wall -DMODULE -D__KERNEL__ -DLINUX -I$(KERNEL_SRC)/include

#$(KERNEL_SRC)/include/linux/version.h
#module.o: module.c 
#	$(CC) $(MODCFLAGS) -c module.c

#all : module.o

v := uname -r
# build for the current host
host : module

module:

	echo `uname -r`
	make -C /usr/src/linux-headers-`uname -r` M=${M}  KCONFIG_CONFIG=/mnt/data/users/lester/projects/linux/i5.config modules