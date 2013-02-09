#test Makefile

#TESTS=openmpi fork pthreads single openmpi2
TESTS=fork pthreads single openmpi2

SOURCES=$(TESTS:=_test.c)



all: $(TESTS)

openmpi: openmpi_test
	./openmpi_test

openmpi_test: openmpi_test.c
	gcc -lgomp -o openmpi_test openmpi_test.c

openmpi2: openmpi2_test
	./openmpi2_test

openmpi2_test: openmpi_test.c
	gcc -lgomp -o openmpi_test openmpi_test.c

sing

fork_test: fork_test.c
	gcc -o fork_test fork_test.c

fork: fork_test
	./fork_test

pthreads_test: pthreads_test.c
	gcc -lpthread -o pthreads_test pthreads_test.c

pthreads: pthreads_test
	./pthreads_test

clean:
	rm *_test
