#!/usr/bin/env python

import sys
from struct import pack, unpack

v7 = 0

class AOut(object) :
	def __init__(self, bin) :
		self.mag, self.txt, self.sym, self.rel, self.dat, self.z = \
			unpack("<HHHHHH", bin[:12])
		assert self.z == 0
		assert self.txt % 2 == 0 and self.sym % 2 == 0 and self.rel % 2 == 0
		self.txtd = bin[:self.txt]
		self.symd = bin[self.txt : self.txt+self.sym]
		self.reld = bin[self.txt+self.sym : self.txt+self.sym+self.rel]
		# XXX why isnt this true?
		#assert len(bin) == self.txt+self.sym+self.rel
	def __str__(self) :
		return '[a.out mag=%x txt=%x sym=%x rel=%x dat=%x z=%x]' % \
			(self.mag, self.txt, self.sym, self.rel, self.dat, self.z)

def dump(fn, addr) :
	f = file(fn, 'rb')
	a = AOut(f.read())
	f.close()
	print a
	assert a.rel == 0 

	vals = unpack("<%dH" % (a.txt/2), a.txtd)

	# it appears the a.out header isnt used for the kernel.
	# dropping the first 16 bytes causes the tables in u0.s to
	# line up with the addresses they were assigned...
	vals = vals[8:]

	for n,v in enumerate(vals) :
		print 'dep cpu %o %06o' % (addr+n*2, v)
	print 'go 400'

# XXX just an example.
#dump('/tmp/s2/bin/ls', 02000)
for fn in sys.argv[1:] :
	dump(fn, 0)

