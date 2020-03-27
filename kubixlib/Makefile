# uLINUX_VER = linux-4.12.8
# uROOT := $(shell echo $$PWD | sed 's%\(.*/buildkernel\)/.*%\1%')
# uBUILD_PATH = $(ROOT)/$(LINUX_VER)/include
OBJCFLAGS +=" -fexceptions"
STDCFLAG +=" -std=c++17"
CFLAGS +=" -fexceptions" -I$(BUILD_PATH) 
CPPFLAGS += -DUNIT_TEST -Wall
MYFLAGS += -DUNIT_TEST

all: lib64/libkubix.so lib/libkubix.a test_dir

lib64/libkubix.so: kubix.o 
	g++ -ggdb3 -fPIC -shared -o $@ $^
lib/libkubix.a: kubix.o
	ar rcs $@ $^	
kubix.o: kubix.cpp kubix.h
	g++ -c -ggdb3 -fPIC $(MYFLAGS) $<
test_dir: 
	cd test && $(MAKE)

clean: 
	find . -exec file {} \; | grep -i "elf\|\bar\b" |\
	   	sed 's%/\(.*\):.*%\/\1%' | xargs rm -f
