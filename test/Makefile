#test Makefile

TESTS=open_mpi fork

all: $(TESTS)

open_mpi: openmpi_test
	./openmpi_test

openmpi_test: openmpi_test.c
	gcc -o openmpi_test openmpi_test.c -lgomp

fork_test: fork_test.c
	gcc -o fork_test fork_test.c

fork:
	./fork | ./fork | ./fork

clean:
	rm *_test
