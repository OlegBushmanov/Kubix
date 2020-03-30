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

ROOT := $(shell echo $$PWD | sed 's%\(.*/kubix\)/.*%\1%')

LIBDIR=\
	$(ROOT)/kubixlib/lib
#	$(ROOT)/kubixlib/lib64


all: bus_test

bus_test: bus_test.o $(LIBDIR)/*.a
	g++ -ggdb3 -o bus_test bus_test.o -pthread -L$(LIBDIR) -lkubix 
bus_test.o: bus_test.cpp
	g++ -c -ggdb3 bus_test.cpp
	
.PHONY: clean
clean:
	rm -f *.o bus_test
