# Makefile for Apout PDP-11 application emulator
#
# $Revision: 1.30 $
# $Date: 2008/05/09 14:04:51 $
#
# You will need gcc if you choose the optimised compile below
CC=gcc

# Set the CFLAGS, LDFLAGS for speed or debugging. If you don't want 2.11BSD
# emulation, then remove the -DEMU211 flag.
# Set up the LIBS if required for your system
#
# These flags for doing debugging
CFLAGS= -Wall -g -DEMU211 -DEMUV1 -DNATIVES -DDEBUG -DZERO_MEMORY -DWRITEBASE
LDFLAGS= -g

# These flags for speed
#CFLAGS= -DEMU211 -DNATIVES -DINLINE=inline -O2 -Winline -Wall \
#	-finline-functions -fomit-frame-pointer
#LDFLAGS=

# Any extra libraries required
LIBS= -lm

# Install destinations
MANDIR=/usr/local/man/man1
BINDIR=/usr/local/bin

VERSION= apout2.3beta1
SRCS=	cpu.c aout.c aout.h branch.c double.c ea.c itab.c main.c ke11a.c \
	single.c fp.c v7trap.c bsdtrap.c defines.h v7trap.h debug.c \
	bsdtrap.h bsd_ioctl.c bsd_signal.c magic.c v1trap.c v1trap.h \
	apout.1 apout.0 README COPYRIGHT CHANGES LIMITATIONS TODO Makefile
OBJS=	aout.o branch.o bsd_ioctl.o bsd_signal.o bsdtrap.o cpu.o debug.o \
	double.o ea.o fp.o itab.o ke11a.o magic.o main.o single.o v1trap.o \
	v7trap.o

apout: $(OBJS)
	cc $(LDFLAGS) $(OBJS) -o apout $(LIBS)

install: apout
	cp apout $(BINDIR)
	chmod 755 $(BINDIR)/apout
	cp apout.1 $(MANDIR)
	chmod 644 $(MANDIR)/apout.1

clean:
	rm -rf apout *core $(OBJS) *.dbg $(VERSION) $(VERSION).tar.gz apout.0

apout.0: apout.1
	nroff -man apout.1 > apout.0

disttar: clean apout.0
	- mkdir $(VERSION)
	cp $(SRCS) $(VERSION)
	chmod -R go+rX $(VERSION)
	chmod -R u+w $(VERSION)
	chown -R wkt $(VERSION)
	tar vzcf $(VERSION).tar.gz $(VERSION)

# Dependencies for object files
aout.o: aout.c defines.h aout.h Makefile
branch.o: branch.c defines.h Makefile
bsd_ioctl.o: bsd_ioctl.c defines.h Makefile
bsd_signal.o: bsd_signal.c defines.h bsdtrap.h Makefile
bsdtrap.o: bsdtrap.c bsdtrap.h defines.h Makefile
cpu.o: cpu.c defines.h Makefile
debug.o: debug.c defines.h Makefile
double.o: double.c defines.h Makefile
ea.o: ea.c defines.h Makefile
fp.o: fp.c defines.h Makefile
itab.o: itab.c defines.h Makefile
ke11a.o: ke11a.c defines.h Makefile
magic.o: magic.c defines.h Makefile
main.o: main.c defines.h Makefile
single.o: single.c defines.h Makefile
v1trap.o: v1trap.c v1trap.h defines.h Makefile
v7trap.o: v7trap.c v7trap.h defines.h Makefile
