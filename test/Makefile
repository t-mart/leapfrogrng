#test Makefile

TESTS=open_mpi fork pthreads

all: $(TESTS)

open_mpi: openmpi_test
	./openmpi_test

openmpi_test: openmpi_test.c
	gcc -o openmpi_test openmpi_test.c -lgomp

fork_test: fork_test.c
	gcc -o fork_test fork_test.c

fork: fork_test
	./fork_test

pthreads_test: pthreads_test.c
	gcc -o pthreads_test pthreads_test.c

pthreads: pthreads_test
	./pthreads_test

clean:
	rm *_test
