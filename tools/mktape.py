#!/usr/bin/env python
"""
Make a tape for 1st ed cold boot.

A sequence of files where each file is started with a metadata block
and followed by data blocks.  A file with a length of zero indicates
the end.

metadata:
    length : word
    mode : byte
    uid : byte
    name : variable length
"""

import os, sys
from struct import pack

def pad(d) :
    "pad to 512-byte boundary"
    l = len(d)
    m = ((l + 511) & ~511) - l
    return d + ("\0" * m)

def sMode(m) :
    return (
        ("-","s")[(m & 040) != 0] +
        ("-","x")[(m & 020) != 0] +
        ("-","r")[(m & 010) != 0] +
        ("-","w")[(m & 004) != 0] +
        ("-","r")[(m & 002) != 0] +
        ("-","w")[(m & 001) != 0] )

def wrFile(out, fn, d, mode, uid) :
    "write out file."
    print sMode(mode), uid, fn
    meta = pack("HBB", len(d), mode, uid) + fn
    out.write(pad(meta))
    out.write(pad(d))

def wrEof(out) :
    wrFile(out, "", "", 0, 0)
    
def copyFile(out, fn) :
    "copy file from local filesystem to tape."
    f = file(fn, 'rb')
    d = f.read()
    f.close()
    s = os.stat(fn)
    mode,uid = s[0] & 0xff, s[4] & 0xff
    if fn[:len(root)] == root :
        fn = fn[len(root):]
    wrFile(out, fn, d, mode, uid)

root = "/tmp/s2/"
def main() :
    f = file("tape", "wb")
    for fn in sys.argv[1:] :
        copyFile(f, fn)
    wrEof(f)
    f.close()

if __name__ == "__main__" :
    main()

