#!/bin/sh
#
# Change that value of APOUT_ROOT below to
# match the location where you unpacked
# the 2nd Edition UNIX binaries
#
APOUT_ROOT=/usr/local/src/V1
export APOUT_ROOT
#
cc="apout $APOUT_ROOT/bin/cc"
as="apout $APOUT_ROOT/bin/as"
cvopt="apout cvopt"
rm -f *.o a.out z c0 c1 cvopt
#
echo Building c00; $cc -c c00.c
echo Building c01; $cc -c c01.c
echo Building c02; $cc -c c02.c
echo Building c03; $cc -c c03.c
echo Building c0t; $as c0t.s; mv a.out c0t.o
echo Building c0;  $cc c0*.o; mv a.out c0; chmod 755 c0
echo
echo Right, thats c0 done
echo
echo Building c10;    $cc -c c10.c
echo Building c11;    $cc -c c11.c
echo Building c1t;    $as c1t.s; mv a.out c1t.o
echo Building cvopt;  $cc cvopt.c; mv a.out cvopt; chmod 755 cvopt
echo Building cctab;  $cvopt < cctab.s > z;  $as z; mv a.out cctab.o
echo Building efftab; $cvopt < efftab.s > z; $as z; mv a.out efftab.o
echo Building regtab; $cvopt < regtab.s > z; $as z; mv a.out regtab.o
echo Building sptab;  $cvopt < sptab.s > z;  $as z; mv a.out sptab.o
echo Building c1;     $cc c1*.o regtab.o efftab.o cctab.o sptab.o
mv a.out c1; chmod 755 c1
echo
echo Right, thats c1 done
echo
echo Building cc; $cc cc.c; mv a.out cc; chmod 755 cc
rm -f *.o a.out z
