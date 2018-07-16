#
# Makefile for building the unix kernel and disk images.
#
# important targets:
#	all		- make kernel and disk images
#	images		- make disk images
#	tape		- make a tape image for a "cold" kernel
#	unix		- make the kernel
#	clean		- delete intermediate files
#	clobber		- delete all generated files
#
# settings:
#	EXTRAPATCHES - list of extra patches to apply
#	USECOREPATCHES - "yes" or "no", use the core set of patches
#
# Examples:
#	make
#		build disk images and unix and run unix in simh
#	make unix tape EXTRAPATCHES=cold
#		build a "cold" kernel and a tape to boot it with.


USECOREPATCHES = yes
EXTRAPATCHES = 

ALL = rf0.dsk rk0.dsk
MKFS = ../tools/mkfs
MKTAPE = ../tools/mktape.py
APOUT_ROOT = ../fs/root
APOUT = APOUT_ROOT=$(APOUT_ROOT) ../tools/apout/apout
AS = $(APOUT) $(APOUT_ROOT)/bin/as
NM = $(APOUT) $(APOUT_ROOT)/bin/nm
TREECOPY = rsync -a --exclude .svn
KSRCS = u0.s u1.s u2.s u3.s u4.s u5.s u6.s u7.s u8.s u9.s ux.s
ALLSRCS = $(KSRCS) init.s sh.s
CLEANSRCS = $(ALLSRCS:.s=.clean)
PAGES = ../pages/e00-01 ../pages/e00-02 ../pages/e00-03 ../pages/e00-04 \
	../pages/e00-05 ../pages/e00-06 ../pages/e00-07 ../pages/e00-08 \
	../pages/e00-09 ../pages/e00-10 ../pages/e01-01 ../pages/e01-02 \
	../pages/e01-03 ../pages/e01-04 ../pages/e01-05 ../pages/e01-06 \
	../pages/e01-07 ../pages/e01-08 ../pages/e01-09 ../pages/e01-10 \
	../pages/e02-01 ../pages/e02-02 ../pages/e02-03 ../pages/e02-04 \
	../pages/e02-05 ../pages/e02-06 ../pages/e02-07 ../pages/e02-08 \
	../pages/e02-09 ../pages/e02-10 ../pages/e03-01 ../pages/e03-02 \
	../pages/e03-03 ../pages/e03-04 ../pages/e04-01 ../pages/e04-02 \
	../pages/e04-03 ../pages/e04-04 ../pages/e04-05 ../pages/e04-06 \
	../pages/e04-07 ../pages/e05-01 ../pages/e05-02 ../pages/e05-03 \
	../pages/e05-04 ../pages/e05-05 ../pages/e05-06 ../pages/e06-01 \
	../pages/e06-02 ../pages/e06-03 ../pages/e06-04 ../pages/e06-05 \
	../pages/e06-06 ../pages/e07-01 ../pages/e07-02 ../pages/e07-03 \
	../pages/e07-04 ../pages/e07-05 ../pages/e07-06 ../pages/e07-07 \
	../pages/e07-08 ../pages/e08-01 ../pages/e08-02 ../pages/e08-03 \
	../pages/e08-04 ../pages/e08-05 ../pages/e08-06 ../pages/e08-07 \
	../pages/e08-08 ../pages/e08-09 ../pages/e09-01 ../pages/e09-02 \
	../pages/e09-03 ../pages/e09-04 ../pages/e09-05 ../pages/e09-06 \
	../pages/e09-07 ../pages/e09-08 ../pages/e10-01 ../pages/e10-02 \
	../pages/e11-01 ../pages/e11-02 ../pages/e11-03 ../pages/e11-04 \
	../pages/e11-05 ../pages/e11-06 ../pages/e11-07 ../pages/e12-01 \
	../pages/e12-02 ../pages/e12-03 ../pages/e12-04

# make the important stuff
all : $(ALL)

# reconstitute sources from ocr'd pages
$(CLEANSRCS) cleansrc : $(PAGES)
	@echo Building clean source...
	@r() { cat ../pages/$$1-* >$$2.clean ; } ; \
		r e00 u0; \
		r e01 u1; \
		r e02 u2; \
		r e03 u3; \
		r e04 u4; \
		r e05 u5; \
		r e06 u6; \
		r e07 u7; \
		r e08 u8; \
		r e09 u9; \
		r e10 ux; \
		r e11 sh; \
		r e12 init
	@touch cleansrc

# patch the clean sources
# XXX what if we do USECOREPATCHES=no and we want to apply
# one of the cores as an EXTRAPATCH?
$(ALLSRCS) patched : $(CLEANSRCS)
	@echo Patching...
	@for src in $(CLEANSRCS) ; do \
		cp $$src `basename $$src .clean`.s; \
	done
	@if [ $(USECOREPATCHES) = "yes" ] ; then \
		for p in ../patches/core/*.patch ; do \
			echo ' ' `basename $$p .patch`;	\
			patch -s -p1 < $$p; \
		done; \
	fi
	@for p in $(EXTRAPATCHES) ; do \
		echo ' ' $$p; \
		patch -s -p1 < ../patches/$$p.patch; \
	done
	@touch patched

# build the unix kernel from the KSRCS using the v2 assembler.
# XXX make ml take cmd line args for input and output file.
unix usyms : $(KSRCS)
	@echo Building kernel...
	@$(AS) u?.s
	@$(NM) a.out | sort > usyms
	@mv a.out unix

# build a prototype filesystems
# XXX has hack to fix our orig hack in sh.s and init.s
root usr protofs : $(ALLSRCS) init.0405 sh.0405
	@echo Building filesystems...
	@$(TREECOPY) ../fs/root .
	@$(TREECOPY) ../fs/new/etc root
	@$(TREECOPY) ../fs/usr .
	@$(TREECOPY) ../fs/new/usr .
	@$(TREECOPY) ../src usr
	@sed '/40014/d' < sh.s > usr/src/cmd/sh.s
	@sed '/40014/d' < init.s > usr/src/cmd/init.s
	@cp $(KSRCS) usr/sys
	@cp init.0405 root/etc/init
	@cp sh.0405 root/bin/sh
	@touch protofs

# build filesystem images
# installs kernel and bootloader on rf0, too.
rf0.dsk rk0.dsk images : protofs unix
	@echo Building disk images...
	@$(MKFS) -p ../fs/Readme root rf0.dsk rf
	@$(MKFS) -p ../fs/Readme usr rk0.dsk rk
	@dd if=../boot/bos of=rf0.dsk bs=512 seek=960
	@# we need to skip the a.out header...
	@dd if=unix bs=1 skip=16 count=16384 | \
	    dd of=rf0.dsk bs=512 seek=964
	@mkdir ../images

# build a tape image
tape : protofs
	@echo Building tape image...
	@$(MKTAPE) root root/bin/* root/etc/*

install : rf0.dsk rk0.dsk
	@echo Installing...
	@cp rf0.dsk rk0.dsk ../boot/m792low.load ../images
	
# clean intermediate files
clean :
	rm -f $(CLEANSRCS) cleansrc
	rm -f $(ALLSRCS) patched *.orig
	rm -f unix usyms
	rm -rf usr root protofs
	rm -f *.0405
	rm -f ../images

# clean intermediate and target files
clobber : clean
	rm -f tape
	rm -f rf0.dsk rk0.dsk


# generic rule for assembling a .s into an 0405 a.out format.
.SUFFIXES : .s .0405
.s.0405 :
	@$(AS) $<
	@../tools/fixaout.py
	@mv a.out $@

