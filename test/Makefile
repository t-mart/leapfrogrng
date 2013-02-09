#test Makefile

TESTS=open_mpi fork

all: $(TESTS)

open_mpi: openmpi_test
	./openmpi_test

openmpi_test: openmpi_test.c
	gcc -o openmpi_test openmpi_test.c -lgomp

fork_test: fork_test.c
	gcc -o fork_test fork_test.c

#redirect so that it's not really piping bc we want to see each output
fork: fork_test
	./fork_test

clean:
	rm *_test
