#!/bin/sh

./a -c connection.conf $* &
./b -c connection.conf $*
