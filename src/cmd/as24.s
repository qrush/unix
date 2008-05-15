/ a4 -- pdp-11 assembler pass 2

oset:
	mov	r2,-(sp)
	mov	(r5)+,r1
	mov	r0,r2
	bic	$!777,r0
	add	r1,r0
	add	$6,r0
	mov	r0,(r1)+	/ next slot
	mov	r1,r0
	add	$1004,r0
	mov	r0,(r1)+	/ buf max
	mov	r2,(r1)+	/ seek addr
	mov	(sp)+,r2
	rts	r5

putw:
	mov	(r5),0f
	jsr	r5,putc; 0:..
	swab	r0

putc:
	mov	r1,-(sp)
	mov	r2,-(sp)
	mov	(r5)+,r2
	mov	(r2)+,r1	/ slot
	cmp	r1,(r2)		/ buf max
	bhis	1f
	movb	r0,(r1)+
	mov	r1,-(r2)
	br	2f
1:
	tst	(r2)+
	mov	r0,-(sp)
	jsr	r5,flush1
	mov	(sp)+,r0
	movb	r0,*(r2)+
	inc	-(r2)
2:
	mov	(sp)+,r2
	mov	(sp)+,r1
	rts	r5

flush:
	mov	(r5)+,r2
	cmp	(r2)+,(r2)+
flush1:
	mov	(r2)+,r1
	mov	r1,0f		/ seek address
	mov	fout,r0
	sys	seek; 0:..; 0
	bic	$!777,r1
	add	r2,r1		/ write address
	mov	r1,0f
	mov	r2,r0
	bis	$777,-(r2)
	inc	(r2)		/ new seek addr
	cmp	-(r2),-(r2)
	sub	(r2),r1
	neg	r1
	mov	r1,0f+2		/ count
	mov	r0,(r2)		/ new next slot
	mov	fout,r0
	sys	write; 0:..; ..
	rts	r5

getc:
	dec	ibufc
	bgt	1f
	movb	fin,r0
	sys	read; inbuf; 512.
	mov	r0,ibufc
	bne	2f
	sev
	rts	pc
2:
	mov	$inbuf,ibufp
1:
	clr	r4
	bisb	*ibufp,r4
	inc	ibufp
	rts	pc

readop:
	mov	savop,r4
	beq	getw
	clr	savop
	rts	pc

getw:
	jsr	pc,getc
	bvs	1f
	mov	r4,-(sp)
	jsr	pc,getc
	bvs	2f
	swab	r4
	bis	(sp)+,r4
	rts	pc
2:
	tst	(sp)+
1:
	mov	$4,r4
	sev
	rts	pc

convs:
	mov	r5,-(sp)
	mov	r4,r5
	clr	r4
	dvd	$40.,r4
	mov	r5,-(sp)
	mov	r4,r5
	clr	r4
	dvd	$40.,r4
	mov	r5,-(sp)
	movb	chartab(r4),r0
	jsr	r5,putc; txtp
	mov	(sp)+,r4
	movb	chartab(r4),r0
	jsr	r5,putc; txtp
	mov	(sp)+,r4
	movb	chartab(r4),r0
	jsr	r5,putc; txtp
	mov	(sp)+,r5
	rts	pc
