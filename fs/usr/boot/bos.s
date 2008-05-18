/ bos -- UNIX V1 bootstrap
/
/ re-creation, based on description in UNIX_ProgammersManual_Nov71.pdf,
/ page 7-06, BOOT PROCEDURES (VII)
/ 5/6/08 jam@magic.com
/
/ M792 bootstrap loads this into core at 54000
/
/ Behavior depends on switch register:
/	173700 or
/	073700	Read Warm UNIX from RF into core location 0 and jump to 400
/	1	Read Cold UNIX from RF into core location 0 and jump to 400
/	2	Read unassigned 3K program into core location 0 and jump to 400
/	10	Dump 12K words from core location 0 onto DECtape drive 7
/	0	UNIMPLEMENTED -- should be read UNIX binary paper tape

csw = 177570

/. = 54000

	mov	$177472,r1
	mov	*$csw,r0
	cmp	r0,$173700
	beq	warm
	cmp	r0,$73700
	bne	1f
warm:
	/ x73700 = Warm UNIX
	mov	$3,-(r1)	/ rf0 dae = track number high bits
	mov	$142000,-(r1)	/ rf0 dar = 256KW - 15KW
	clr	-(r1)		/ rf0 cma = 0
	mov	$-14000,-(r1)	/ rf0 wc = 6KW
loadrf:
	mov	$5,-(r1)	/ rf0 dcs = read
	tstb	(r1)		/ rf0 done?
	bge	.-2		/ no, loop
	jmp	*$400		/ jump to loaded code

1:
	cmp	r0,$1
	bne	1f

	/ 0 = Cold UNIX
	mov	$3,-(r1)	/ rf0 dae = track number high bits
	mov	$156000,-(r1)	/ rf0 dar = 256KW - 9KW
	clr	-(r1)		/ rf0 cma = 0
	mov	$-14000,-(r1)	/ rf0 wc = 6KW
	br	loadrf

1:
	cmp	r0,$2
	bne	1f

	/ 2 = Unassigned 3K
	mov	$3,-(r1)	/ rf0 dae = track number high bits
	mov	$172000,-(r1)	/ rf0 dar = 256KW - 3KW
	clr	-(r1)		/ rf0 cma = 0
	mov	$-6000,-(r1)	/ rf0 wc = 3KW
	br	loadrf

1:
	cmp	r0,$10
	bne	1f

	/ 10 = Dump 12K to DECtape drive 7
	mov	$177350,r1
	clr	-(r1)		/ tc11 tcba = 0
	mov	$-30000,-(r1)	/ tc11 tcwc = 12KW
	mov	$3415,-(r1)	/ tc11 tccm = UNIT=7,WDATA,DO
	tstb	(r1)		/ tc11 done?
	bge	.-2		/ no, loop
2:
	0			/ halt
	br	2b

1:
	0			/ halt -- unrecognized switch setting
	br	1b
