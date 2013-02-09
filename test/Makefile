#test Makefile

#TESTS=openmpi fork pthreads single openmpi2
TESTS=fork pthreads single openmpi2

HEADERS=test.h

LIBS=-lgomp -lpthread -fopenmp

#.PHONY: run_*

all: $(addprefix run_,$(TESTS))

%.o: %.c $(HEADERS)
	gcc $(LIBS) -o $@ $<

run_%: %.o %.c
	./$<
