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

# include libgcc_s in path 
# /etc/ld.so.conf.d/ add libgcc.conf file with line /lib/i386-linux-gnu/
# run ldconfig

CPPFLAGS += -finput-charset=ascii -Wall -Wextra -pedantic -Wmissing-declarations -Wpointer-arith -Wwrite-strings -Wformat=2 -Wlogical-op -Wcast-align -Wcast-qual -Wundef -Wmissing-include-dirs -Wfloat-equal -Wconversion
CPPFLAGS += --std=c++11 -Wall
#CPPFLAGS += -include ../cpputest/include/CppUTest/MemoryLeakDetectorNewMacros.h
#CPPFLAGS += -include ../cpputest/include/CppUTest/MemoryLeakDetectorMallocMacros.h
CPPFLAGS += -Wl,-Bstatic -lCppUTest -Wl,-Bstatic -lCppUTestExt -Wl,-Bdynamic -lgcc_s
#CPPFLAGS += -L../cpputest/lib
#CPPFLAGS += -I../cpputest/include/CppUtest -I../cpputest/include/CppUtestExt
#CPPFLAGS += -DMTEST_BUILD -DCPPUTEST_USE_MEM_LEAK_DETECTION=1
CPPFLAGS += -lpthread  
#LD_LIBRARIES += -L../cpputest/lib -lCppUTest -lCppUTestExt

module:
	echo `uname -r`
	make -C /usr/src/linux-headers-`uname -r` M=$(CURDIR) ${options}  modules

test: mtest/main.cpp mtest/queue.cpp ../cpp-lib/posix/File.cpp queue.h
	g++ -g -o test  $(CPPFLAGS)   $^
	
utest:
#-Wl,-Bstatic -lCppUTest -Wl,-Bstatic -lCppUTestExt

tsc_test: tsc_test.cpp ../cpp-lib/posix/File.cpp
	g++ -g -o tsc_test --std=c++11 $^
		
clean:
	rm -f *.ko *.mod.* *.order *.o test