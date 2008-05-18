/ msys2 -- copy file to RF read only slot
/
/ re-creation, based on description in UNIX_ProgammersManual_Nov71.pdf,
/ page 7-06, BOOT PROCEDURES (VII)
/ 5/9/08 jam@magic.com
/ 5/17/08 jam@magic.com -- modified to copy 407-format a.out files

/ b bos		1700	1KW
/ u warm unix	1704	6KW
/ 1 cold unix	1734	6KW
/ 2 unassigned	1764	3KW

	mov	sp,r5
	mov	(r5)+,r3	/ argc
	cmp	$3,r3		/ must be 3
	bne	badcmd		/ else error
	tst	(r5)+
	mov	(r5)+,r4	/ get first arg

	cmpb	(r4),$'b
	bne	1f
	mov	$1700,r3
	mov	$4,r4
	br	2f
1:
	cmpb	(r4),$'u
	bne	1f
	mov	$1704,r3
	mov	$30,r4
	br	2f
1:
	cmpb	(r4),$'1
	bne	1f
	mov	$1734,r3
	mov	$30,r4
	br	2f
1:
	cmpb	(r4),$'2
	bne	badcmd
	mov	$1764,r3
	mov	$14,r4
2:

	/ open file
	mov	(r5),r5
	mov	r5,0f
	sys	open; 0:..; 0
	bes	error
	mov	r0,r1
	sys	seek; 20; 0	/ skip 407 (16-byte) header
	bes	error

	/ open rf0 and seek to correct block
	sys	open; disk; 1
	bes	error
	mov	r0,r2
	mov	r3,0f
	sys	seek; 0:..; 0
	bes	error

	/ copy file from file to disk one block at a time
1:
	mov	r1,r0
	sys	read; buf; 512.
	mov	r0,r5
	mov	r2,r0
	sys	write; buf; 512.
	bes	error
	dec	r4
	beq	3f
	tst	r5
	bne	1b

3:
	sys	exit

error:
	mov	$1,r0
	sys	write; 1f; 2
	br	.+2
	sys	exit
1:
	<?\n>

badcmd:
	mov	$1,r0
	sys	write; 1f; 2
	br	.+2
	sys	exit
1:
	<?\n>

disk:
	</dev/rf0\0>
	.even

buf:	.=.+512.
