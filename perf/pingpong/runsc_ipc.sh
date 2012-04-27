#!/bin/sh

./a -p Pingpong.spr -s hostfile $* &
./b -p Pingpong.spr -s hostfile $*
