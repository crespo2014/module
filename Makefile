#KERNEL_SRC must be define

#one module multiple objects
#module-objs := readmap.o

#obj-m := module.o 
#obj-m := tsc.o
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
options := CONFIG_DEBUG_SECTION_MISMATCH=y

ifdef CONFIG
 options += KCONFIG_CONFIG=${CONFIG}
endif

v := uname -r
# build for the current host
host : module

module:
	echo `uname -r`
	make -C /usr/src/linux-headers-`uname -r` M=$(CURDIR) ${options}  modules

test: mtest/queue.cpp ../cpp-lib/posix/File.cpp queue.h
	g++ -g -o test -lpthread --std=c++11 $^
	

tsc_test: tsc_test.cpp ../cpp-lib/posix/File.cpp
	g++ -g -o tsc_test --std=c++11 $^
		
clean:
	rm -f *.ko *.mod.* *.order *.o test