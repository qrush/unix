#!/bin/sh
#
# build a tape for simh containing the kernel sources
# requires conv2.c to be built

UNIXDIR=~/work/simh/unix-v7-4/run

tools/rebuild
(cd rebuilt; gtar -O -cf ../u.tar u?.s)
tools/conv2 -o tape.tm u.tar
cp tape.tm $UNIXDIR
