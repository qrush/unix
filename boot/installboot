#!/bin/sh

RF0IMAGE=rf0.dsk
KERNEL=build/loadfile
BOS=boot/bos

if [ ! -f $KERNEL ]
then echo "You must build the kernel, $KERNEL, before you can install"
     echo "the bootstrap."
     exit 1
fi

# Install bos 32KW below the top of the rf0 image
#
dd if=$BOS of=$RF0IMAGE bs=512 seek=960

# Install Unix in the warm boot area, 31KW below the top of the
# rf0 image.  Because the kernel is in simh load format, the first
# 6 bytes are skipped and the following 16K byte are copied onto
# the rf0 disk.
#
dd if=$KERNEL bs=1 skip=6 count=16384 | dd of=$RF0IMAGE bs=512 seek=964
