#KERNEL_SRC must be define
CC=gcc

MODCFLAGS := -Wall -DMODULE -D__KERNEL__ -DLINUX -I$(KERNEL_SRC)/include

#$(KERNEL_SRC)/include/linux/version.h
module.o: module.c 
	$(CC) $(MODCFLAGS) -c module.c

all : module.o