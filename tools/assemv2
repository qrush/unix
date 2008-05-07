#!/bin/sh
#
# assemble the sources
#
if [ ! -d tools ]
then echo 'No tools/ directory, are you running this in the correct dir?'
     exit 1
fi
if [ ! -f tools/pdp11 ]
then echo 'You need to compile Simh with the ke11 patch, and place the'
     echo 'resulting pdp11 executable into the tools/ directory.'
     exit 1
fi
if [ ! -f tools/apout/apout ]
then echo 'You need to go into tools/apout/ and do a make to compile apout'
     exit 1
fi
if [ ! -d build ]
then mkdir build
fi

APOUT=../tools/apout/apout
APOUT_ROOT=../fs/root
export APOUT_ROOT

# Build sources from pages and generate patched sources in "build".
# Any command-line args are names of patch files in patches/, but without
# the trailing .patch. If "nopatch" is the first command-line argument,
# then no patches will be applied, and build/ is expected to already have
# the patched kernel code.
if [ ! "$1" = "nopatch" ]
then tools/rebuild "$@"
fi

# assemble the kernel from patched sources and generate symbols
# and build a simh loadable file.
cd build

# assemble it all
$APOUT $APOUT_ROOT/bin/as u?.s
$APOUT $APOUT_ROOT/bin/nm a.out |sort > a.out.syms
../tools/ml

