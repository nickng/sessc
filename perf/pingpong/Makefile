ROOT := ../..
include $(ROOT)/Common.mk

all: a b mpi zmq_a zmq_b

%: %.c
	$(CC) $(CFLAGS) -o $* $*.c $(LDFLAGS)

mpi: mpi.c
	mpicc $(CFLAGS) -o mpi mpi.c $(LDFLAGS)

clean:
	rm a b mpi zmq_a zmq_b
