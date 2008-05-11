/ UNIX bootstrap ROM, based on BOOT PROCEDURES (VII) 11/3/71

	. = 173700

	/ 173700 -- load bos from 1700000 of rf0 (RF11)

	mov	$177472,r0
	mov	$3,-(r0)	/ rf11.dae.ta[6:5] = 3
	mov	$140000,-(r0)	/ rf11.dar = 140000
	mov	$54000,-(r0)	/ rf11.cma = 54000
	mov	$-2000,-(r0)	/ rf11.wc = -2000 (1K words)
	mov	$5,-(r0)	/ rf11.dcs = read,go
	tstb	(r0)		/ done?
	bge	.-2		/ no, loop
	jmp	*$54000		/ jump to bos

	/ 173740 -- load from tap0 (TC11 DECtape)

	mov	$177350,r0
	clr	-(r0)		/ tc11.tcba = 0
	mov	r0,-(r0)	/ tc11.tcwc = 177346 (282. bytes)
	mov	$3,-(r0)	/ tc11.tccm = rnum,do
	tstb	(r0)		/ done?
	bge	.-2		/ no, loop
	tst	*$177350	/ block 0?
	bne	.		/ no, loop forever
	movb	$5,(r0)		/ tc11.tccm = rdata,do
	tstb	(r0)		/ done?
	bge	.-2		/ no, loop
	clr	pc		/ jump to location 0
