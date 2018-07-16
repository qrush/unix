#!/usr/bin/env python2
"""
Convert an 0407 binary into an 0405 binary, under the assumption
that the code starts at 040014 (by ".. = 40014").
See tools/as.
"""

import struct

def words(bs) :
    l = len(bs) / 2
    return list(struct.unpack('<%dH' % l, bs))
def unwords(ws) :
    l = len(ws)
    return struct.pack('<%dH' % l, *ws)

def read(fn) :
    f = file(fn, 'rb')
    d = f.read()
    f.close()
    return d

def write(fn, d) :
    f = file(fn, 'wb')
    f.write(d)
    f.close()

d1 = words(read('a.out'))
hdr = d1[:8]
d = [0405, 12+hdr[1], 0, 0, hdr[4], 0] + d1[8:]
write("a.out", unwords(d))
