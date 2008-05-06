/ maki -- make initialization dec tape

	sys	open; tape; 1
	bes	error
	mov	r0,fo
	sys	write; vcboot; 512.
	bes	error
	sys	open; disk; 0
	bes	error
	mov	r0,r1
	sys	seek; 1700; 0
	bes	error
	mov	$64.,r2
1:
	mov	r1,r0
	sys	read; buf; 512.
	jsr	pc,tout
	dec	r2
	bne	1b
	mov	r1,r0
	sys	close
	mov	$files,r5
1:
	tstb	(r5)
	beq	1f
	mov	r5,0f
	sys	stat; 0:..; buf
	bes	error
	mov	$buf,r3
	mov	buf+6,(r3)+	/ size
	movb	buf+2,(r3)+	/ mode
	movb	buf+5,(r3)+	/ uid
2:
	movb	(r5)+,(r3)+
	bne	2b
2:
	clrb	(r3)+
	cmp	r3,$buf+512.
	blo	2b
	jsr	pc,tout
	sys	open; buf+4; 0
	bes	error
	mov	r0,fi
2:
	mov	fi,r0
	sys	read; buf; 512.
	tst	r0
	beq	2f
	jsr	pc,tout
	br	2b
2:
	mov	fi,r0
	sys	close
	br	1b
1:
	clr	buf
	jsr	pc,tout
	sys	exit

tout:
	mov	fo,r0
	sys	write; buf; 512.
	bes	error
	rts	pc

error:
	mov	$1,r0
	sys	write; 1f; 4
	4
	sys	exit
1:
	<?\n>

vcboot:
	mov	$20000,sp
	jsr	r5,dtio; 1; 20000; -20000; 5
	jsr	r5,drio; 3; 140000; 20000; -20000; 3
	jsr	r5,dtio; 33.; 20000; -20000; 5
	jsr	r5,drio; 3; 160000; 20000; -20000; 3
	jsr	r5,drio; 3; 140000; 54000; -2000; 5
	jmp	*$54000

tcdt = 177350
tccm = 177342
dtio:
	mov	$tcdt,r0
	mov	$tccm,r1
	mov	$3,(r1)	/ rn-f
1:
	tstb	(r1)
	bge	1b
	tst	(r1)
	blt	0f
	cmp	(r5),(r0)
	beq	2f
	bgt	dtio
0:
	mov	$4003,(r1)	/ rn-b
1:
	tstb	(r1)
	bge	1b
	tst	(r1)
	blt	dtio
	mov	(r0),r2
	add	$5,r2
	cmp	(r5),r2
	bgt	dtio
	br	0b
2:
	tst	(r5)+
	mov	(r5)+,-(r0)
	mov	(r5)+,-(r0)
	mov	(r5)+,-(r0)
1:
	tstb	(r0)
	bge	1b
	tst	(r0)
	bge	1f
	sub	$8.,r5
	br	dtio
1:
	mov	$1,(r0)		/ sat
	rts	r5

dae = 177470
drio:
	mov	$dae+2,r0
	mov	(r5)+,-(r0)
	mov	(r5)+,-(r0)
	mov	(r5)+,-(r0)
	mov	(r5)+,-(r0)
	mov	(r5)+,-(r0)
1:
	tstb	(r0)
	bge	1b
	tst	(r0)
	bge	1f
	sub	$10.,r5
	br	drio
1:
	rts	r5

tape:
	</dev/tap7\0>
disk:
	</dev/rf0\0>

files:
	</etc/init\0>
	</etc/getty\0>
	</bin/chmod\0>
	</bin/date\0>
	</bin/login\0>
	</bin/mkdir\0>
	</bin/sh\0>
	</bin/tap\0>
	</bin/ls\0>
	<\0>
	.even
fi:	.=.+2
fo:	.=.+2
buf:	.=.+512.
