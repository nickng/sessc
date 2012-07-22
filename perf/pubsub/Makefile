ROOT := ../..
include $(ROOT)/Common.mk

all: mpi zmq0 zmq1

%: %.c
	$(CC) $(CFLAGS) -o $* $*.c $(LDFLAGS)

mpi: mpi.c
	mpicc $(CFLAGS) -o mpi mpi.c $(LDFLAGS)

clean:
	rm mpi zmq0 zmq1
