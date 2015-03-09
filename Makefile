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


# targets define different output targets.
# <target_name>_src_dir define directory containing all file for target
# <target_name>_src     define cpp and c files for target
# <target>_cpp    define all cpp flags for target
# <target>_ld     define all ld flags for target
# common_cpp	define common cpp flags
# common_ld		define common ld flags
# src_dirs      all directories containing source files
# srcs 			all sources files

OBJCOPY := objcopy

build_dir := build
src_dirs := mtest cpp-lib/posix
srcs := 

targets := queue_mtest

common_cpp := --std=c++11 -Wall
common_ld := -lpthread
common_cpp += -finput-charset=ascii -Wall -Wextra -pedantic -Wmissing-declarations -Wpointer-arith -Wwrite-strings -Wformat=2 -Wlogical-op -Wcast-align -Wcast-qual -Wundef -Wmissing-include-dirs -Wfloat-equal -Wconversion


queue_mtest_src_dir := mtest
queue_mtest_src := cpp-lib/posix/File.cpp
queue_mtest_cpp := -I../cpputest/include
queue_mtest_cpp += -include ../cpputest/include/CppUTest/MemoryLeakDetectorNewMacros.h
queue_mtest_cpp += -include ../cpputest/include/CppUTest/MemoryLeakDetectorMallocMacros.h
queue_mtest_ld := -Wl,-Bstatic -lCppUTest -Wl,-Bstatic -lCppUTestExt -Wl,-Bdynamic -lgcc_s  -L../cpputest/lib


#Look for all cpp files in a folder and return as build/%.o
find-cpp-o =  $(addprefix $(build_dir)/,$(patsubst %.cpp,%.o,$(foreach d,$(1),$(wildcard $(d)/*.cpp))))

#Look for all c files in a folder and return as build/%.o
find-c-o =  $(addprefix $(build_dir)/,$(patsubst %.c,%.o,$(foreach d,$(1),$(wildcard $(d)/*.c))))

#Convert a list containing c and cpp files to  build/%.o
get-o = $(addprefix $(build_dir)/,$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$1)))

#Look for all c and cpp files in a folder and return as build/%.o
find-o = $(call find-cpp-o,$(1)) $(call find-c-o,$(1))

c_files := $(foreach d,$(src_dirs),$(wildcard $(d)/*.c))
cpp_files := $(foreach d,$(src_dirs),$(wildcard $(d)/*.cpp))

# List of all targets that will be built, with path of build directory prepended
all:   $(addprefix $(build_dir)/,$(targets)) 
debug: $(addsuffix _debug, $(addprefix $(build_dir)/,$(targets)))
coverage: $(addsuffix _coverage, $(addprefix $(build_dir)/,$(targets)))
asan: $(addsuffix _asan, $(addprefix $(build_dir)/,$(targets)))
tsan: $(addsuffix _tsan, $(addprefix $(build_dir)/,$(targets)))
clang: $(addsuffix _clang, $(addprefix $(build_dir)/,$(targets)))

all_suffix := debug coverage asan tsan clang profiler

# ****** define a rule to build all target for and specific suffix ******
# 1 - suffix

define suffix-rule

$1 : $(addprefix $(build_dir)/,$(addsuffix _$(1),$(targets)))

endef 

# ******************************************************************************************************
# Define a rule to build a target
# 1 - target name	
# 2 - suffix
define target-rule 

$(build_dir)/$(1)_$(2) : $(addsuffix .$(1)_$(2),$(call find-o,$($(1)_src_dir)) $(call get-o,$($(1)_src)))
	$(Q)echo "$$(@F)"
	$(Q)$(CXX) -o $$@ $$^ $(common_ld) $($(1)_ld)  $($(2)_ld)
	
# alias for target name	
$(1)_$(2): $(build_dir)/$(1)_$(2) 
#$(1).$(2): $(build_dir)/$(1)_$(2) 

endef

# ******************************************************************************************************
# Define a rule to build define target without suffix 
# 1 - target name	
define main-target-rule

$(build_dir)/$(1).dbg : $(addsuffix .$(1)_,$(call find-o,$($(1)_src_dir)) $(call get-o,$($(1)_src))) 
#	$(Q)echo "$$(@F)"
	$(Q)$(CXX) -o $$@ $$^ $(common_ld) $($(1)_ld) 

$(build_dir)/$(1) : $(build_dir)/$(1).dbg
	$(Q)echo "$$(@F)"  
	$(Q)$(OBJCOPY) -S $(build_dir)/$(1).dbg $(build_dir)/$(1)
	$(Q)cd $(build_dir); $(OBJCOPY) --add-gnu-debuglink=$(1).dbg $(1)

# alias for target name	
$(1): $(build_dir)/$(1)

endef

# ******************************************************************************************************	
# Define a rule to build a cpp file for a specific target 
# 1 - file name 
# 2 - target
# 3 - suffix
define cpp-file-target-rule

$(build_dir)/$(1:%.cpp=%.o).$(2)_$(3): $(1) |  $(dir $(build_dir)/$(1))
 
#	$(Q)echo "$(build_dir)/$(1).$(2)$(3).d"
	$(Q)$(CXX) -MM $($(3)_cpp) $(common_cpp) $($(2)_cpp) -MT "$(build_dir)/$(1:%.cpp=%.o).$(2)_$(3)" -MMD -MP -MF "$(build_dir)/$(1).$(2)_$(3).d"  $(1)
	
	$(Q)echo "$$(@F)"
	$(Q)$(CXX) -c $($(3)_cpp) $(common_cpp) $($(2)_cpp) -o $$@  $(1)

-include $(build_dir)/$(1).$(2)_$(3).d

endef

# ******************************************************************************************************
# Define a rule to build a c file for a specific target 
# 1 - file name 
# 2 - target
# 3 - suffix
define c-file-target-rule 

$(build_dir)/$(1:%.c=%.o).$(2)_$(3): $(1)  |  $(dir $(build_dir)/$(1))

#	$(Q)echo "$(build_dir)/$(1).$(2)$(3).d"
	$(Q)$(CC) -MM $($(3)_cpp) $(common_cpp) $($(2)_cpp) -MT "$(build_dir)/$(1:%.c=%.o).$(2)_$(3)" -MMD -MP -MF "$(build_dir)/$(1).$(2)_$(3).d"  $(1)
	
	$(Q)echo "$$(@F)"
	$(Q)$(CC) -c  $($(3)_cpp) $(common_cpp) $($(2)_cpp)  -o $$@  $(1)

-include $(build_dir)/$(1).$(2)_$(3).d

endef

# this flags are use for c and cpp files when building normal target
_cpp := -O3 -g -DNDEBUG

# Compile with flags to include info that can be used with valgrind or gdb.
debug_cpp := -g -D_DEBUG

# Compile with flags to include info that can be used with gcov (code coverage analysis)
coverage_cpp := -O0 -g -fprofile-arcs -ftest-coverage --coverage
coverage_ld := -fprofile-arcs

# Compile with flags to include AddressSanitizer (see http://code.google.com/p/address-sanitizer/)
asan_cpp := -O1 -g  -fno-omit-frame-pointer
asan_cpp += -fsanitize=address

# Compile with flags to include ThreadSanitizer (see http://code.google.com/p/data-race-test/wiki/ThreadSanitizer)
_FLAGS.tsan := -O1 -g  -fPIE -fno-omit-frame-pointer
_CPPFLAGS.tsan := -fsanitize=thread

# Compile with Clang specific runtime checks (http://clang.llvm.org/docs/UsersManual.html#controlling-code-generation)
_FLAGS.clang := -O1 -g -fno-omit-frame-pointer
_FLAGS.clang += -Wno-missing-noreturn -Wno-documentation-unknown-command -Weverything -Wno-padded -Wno-documentation
_FLAGS.clang += -Wno-disabled-macro-expansion -Wno-unknown-warning-option -Wno-format-nonliteral $(CFLAGS_gcc)
_CPPFLAGS.clang := -fsanitize=integer,undefined 

#Compiler for profiling
_FLAGS.profiler := -pg -O1
_LDFLAGS.profiler := -pg -O1

#define a rule for each suffix
$(foreach t,$(all_suffix),$(eval $(call suffix-rule,$(t))))

#defining main target
$(foreach t,$(targets),$(eval $(call main-target-rule,$(t))))
$(foreach t,$(targets), $(foreach f,$(cpp_files), $(eval $(call cpp-file-target-rule ,$(f),$(t)))))
$(foreach t,$(targets), $(foreach f,$(c_files), $(eval $(call c-file-target-rule ,$(f),$(t)))))

#defining extra target using as debug clang, etc
$(foreach t,$(targets),$(foreach s,$(all_suffix),$(eval $(call target-rule ,$(t),$(s)))))
$(foreach t,$(targets),$(foreach f,$(cpp_files) ,$(foreach s,$(all_suffix) ,$(eval $(call cpp-file-target-rule ,$(f),$(t),$(s))))))
$(foreach t,$(targets),$(foreach f,$(c_files)   ,$(foreach s,$(all_suffix) ,$(eval $(call c-file-target-rule   ,$(f),$(t),$(s))))))

# all targets depends on directory
$(addprefix $(build_dir)/,$(targets)): | $(@D)

#rule to create build directory
$(build_dir): 
	mkdir -p $@
	 
#rule to create build subdirectories
$(addsuffix /,$(addprefix $(build_dir)/,$(src_dirs))) : | $(build_dir)
	mkdir -p $@  



# include libgcc_s in path 
# /etc/ld.so.conf.d/ add libgcc.conf file with line /lib/i386-linux-gnu/
# run ldconfig

test : $(targets)

clean:
	rm -rf $(build_dir)
	rm -f *.ko *.mod.* *.order *.o test