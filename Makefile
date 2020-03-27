LDLIBS=../lib
all: bus_test

bus_test: bus_test.o
	g++ -ggdb3 -o bus_test -pthread -L../lib64 -lkubix bus_test.o
bus_test.o: bus_test.cpp
	g++ -c -ggdb3 bus_test.cpp
	
.PHONY: clean
clean:
	rm *.o bus_test
