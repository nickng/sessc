#!/bin/sh

mpirun -np 1 ./a-mpi -p Pingpong.spr $* :\
       -np 1 ./b-mpi -p Pingpong.spr $*
