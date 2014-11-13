#KERNEL_SRC must be define

#one module multiple objects
#module-objs := readmap.o

obj-m := module.o 

#multiple modules
#obj-m := module.o readmap.o

#CC=gcc

#MODCFLAGS := -Wall -DMODULE -D__KERNEL__ -DLINUX -I$(KERNEL_SRC)/include

#$(KERNEL_SRC)/include/linux/version.h
#module.o: module.c 
#	$(CC) $(MODCFLAGS) -c module.c

#all : module.o