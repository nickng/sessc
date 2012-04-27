#!/bin/sh

echo MPI TCP
echo
./runmpi.sh $*
echo
echo ZMQ TCP
echo
./runzmq.sh $*
echo
echo Session C IPC
echo
./runsc_ipc.sh $*
sleep 5
echo
echo Session C TCP
echo
./runsc_tcp.sh $*
