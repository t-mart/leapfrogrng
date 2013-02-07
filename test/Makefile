#test Makefile

TESTS=open_mpi

all: $(TESTS)

open_mpi: openmpi_test
	./openmpi_test

openmpi_test: openmpi_test.c
	gcc -o openmpi_test openmpi_test.c -lgomp

clean:
	rm *_test
