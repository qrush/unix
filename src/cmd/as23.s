/ a3 -- pdp-11 assembler pass 2

assem:
	jsr	pc,readop
	jsr	pc,checkeos
		br ealoop
	mov	r4,-(sp)
	jsr	pc,readop
	cmp	(sp),$1
	bne	1f
	mov	$2,(sp)
	mov	r4,numval
	jsr	pc,readop
1:
	cmp	r4,$'=
	beq	4f
	cmp	r4,$':
	beq	1f
	mov	r4,savop
	mov	(sp)+,r4
	jsr	pc,opline
	br	ealoop
1:
	mov	(sp)+,r4
	cmp	r4,$200
	bhis	1f
	cmp	r4,$2
	beq	3f
	jsr	r5,error; 'x
	br	assem
1:
	cmp	dot,symtab+2(r4)
	beq	assem
	jsr	r5,error; 'p
	br	assem
3:
	mov	numval,r4
	jsr	pc,fbadv
	br	assem
4:
	jsr	pc,readop
	jsr	pc,expres
	mov	(sp)+,r1
	cmp	r1,$200	/test for dot
	bne	1f
	bic	$40,r3
	cmp	r3,dot-2	/ can't change relocation
	bne	2f
	cmp	r3,$4		/ bss
	bne	3f
	mov	r2,dot
	br	ealoop
3:
	sub	dot,r2
	bmi	2f
	mov	r2,-(sp)
3:
	dec	(sp)
	bmi	3f
	clr	r2
	mov	$1,r3
	jsr	pc,outb
	br	3b
3:
	tst	(sp)+
	br	ealoop
2:
	jsr	r5,error; '.
	br	ealoop
1:
	cmp	r3,$40
	bne	1f
	jsr	r5,error; 'r
1:
	bic	$37,symtab(r1)
	bic	$!37,r3
	bne	1f
	clr	r2
1:
	bisb	r3,symtab(r1)
	mov	r2,symtab+2(r1)

ealoop:
	cmp	r4,$'\n
	beq	1f
	cmp	r4,$'\e
	bne	assem
	rts	pc
1:
	inc	line
	br	assem

checkeos:
	cmp	r4,$'\n
	beq	1f
	cmp	r4,$';
	beq	1f
	cmp	r4,$'\e
	beq	1f
	add	$2,(sp)
1:
	rts	pc

fbadv:
	movb	nxtfbr(r4),curfbr(r4)
	asl	r4
	mov	nxtfb(r4),curfb(r4)
	mov	nxtfbp(r4),r1
	bne	1f
	mov	fbbufp,r1
1:
	cmpb	r4,(r1)+
	beq	1f
	tstb	(r1)+
	bpl	2f
	dec	r1
	br	1f
2:
	tst	(r1)+
	br	1b
1:
	movb	(r1)+,r0
	mov	(r1)+,nxtfb(r4)
	mov	r1,nxtfbp(r4)
	asr	r4
	movb	r0,nxtfbr(r4)
	rts	pc

