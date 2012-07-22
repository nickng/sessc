#!/bin/sh

echo MPI TCP
echo
mpirun -np 2 ./mpi $*
echo
echo ZMQ TCP
echo
./runzmq.sh $*
echo
