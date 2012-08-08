ROOT := ../..
include $(ROOT)/Common.mk

all: a b mpi zmq_a zmq_b a-mpi b-mpi

a-mpi: a-mpi.c
	mpicc $(CFLAGS) -o a-mpi a-mpi.c -L$(LIB_DIR) -lsc-mpi

b-mpi: b-mpi.c
	mpicc $(CFLAGS) -o b-mpi b-mpi.c -L$(LIB_DIR) -lsc-mpi

%: %.c
	$(CC) $(CFLAGS) -o $* $*.c $(LDFLAGS)

mpi: mpi.c
	mpicc $(CFLAGS) -o mpi mpi.c $(LDFLAGS)

clean:
	rm a b mpi zmq_a zmq_b
