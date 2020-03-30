#################################################################################
# 	kubix.cpp
# 
# 2020+ Copyright (c) Oleg Bushmanov <olegbush55@hotmai.com>
# All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#################################################################################

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
