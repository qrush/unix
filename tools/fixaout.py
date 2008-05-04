#!/usr/bin/env python
"""
Convert an 0407 binary into an 0405 binary, under the assumption
that the code starts at 040014 (by ". = . + 40014").
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

orig = 040000
d = words(read('a.out'))
hdr = d[:8]
d = d[8 + orig/2 :]

d[0] = 0405
d[1] = hdr[1] - orig
d[2] = 0
d[3] = 0
d[4] = hdr[4]
d[5] = 0
d = d[:d[1] / 2]

write("b.out", unwords(d))
