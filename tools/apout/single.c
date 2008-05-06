/* single.c - Single operand instructions.
 *
 * $Revision: 2.10 $
 * $Date: 1999/01/05 23:46:04 $
 */
#include "defines.h"

/* adc() - Add Carry Instruction. */
void 
adc()
{
    load_dst();

    if (CC_C) {		/* do if carry is set */
	if (dstword == MPI)
	    SET_CC_V();
	else
	    CLR_CC_V();
	if (dstword == NEG_1)
	    SET_CC_C();
	else
	    CLR_CC_C();
	dstword++;			/* add the carry */
    } else {
	CLR_CC_V();
	CLR_CC_C();
    }

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);

    store_dst_2();
}


/* asl() - Arithmetic Shift Left Instruction. */
void 
asl()
{
    load_dst();

    if (dstword & SIGN)
	SET_CC_C();
    else
	CLR_CC_C();

    dstword <<= 1;

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CHG_CC_V_XOR_C_N();

    store_dst_2();
}

/* asr() - Arithmetic Shift Right Instruction. */
void 
asr()
{
    load_dst();

    if (dstword & LSBIT)
	SET_CC_C();
    else
	CLR_CC_C();

    dstword = (dstword >> 1) + (dstword & SIGN);     /* shift and replicate */

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);

    CHG_CC_V_XOR_C_N();

    store_dst_2();
}

/* clr() - Clear Instruction. */
void 
clr()
{
    CLR_CC_ALL(); SET_CC_Z();
    dstword=0; store_dst();
}

/* com() - Complement Instruction. */
void 
com()
{
    load_dst();

    dstword = ~dstword;

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CLR_CC_V();
    SET_CC_C();

    store_dst_2();
}

/* dec() - Decrement Instruction. */
void 
dec()
{
    load_dst();

    if (dstword == MNI)
	SET_CC_V();
    else
	CLR_CC_V();

    --dstword;

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);

    store_dst_2();
}

/* inc() - Increment Instruction. */
void 
inc()
{
    load_dst();

    if (dstword == MPI)
	SET_CC_V();
    else
	CLR_CC_V();

    ++dstword;

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);

    store_dst_2();
}

/* neg() - Negate Instruction. */

void 
neg()
{
    load_dst();

    dstword = (NEG_1 - dstword) + 1;

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);

    if (dstword == MNI)
	SET_CC_V();
    else
	CLR_CC_V();

    if (dstword == 0)
	CLR_CC_C();
    else
	SET_CC_C();

    store_dst_2();
}

/* rol() - Rotate Left Instruction. */
void 
rol()
{
    load_dst();

    tmpword = dstword & SIGN;		/* get sign bit */
    dstword <<= 1;			/* shift */

    if (CC_C)		/* roll in carry */
	dstword += LSBIT;

    if (tmpword)			/* roll out to carry */
	SET_CC_C();
    else
	CLR_CC_C();

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CHG_CC_V_XOR_C_N();

    store_dst_2();
}


/* ror() - Rotate Right Instruction. */
void 
ror()
{
    load_dst();

    tmpword = dstword & LSBIT;	/* get low bit */
    dstword >>= 1;			/* shift */

    if (CC_C)		/* roll in carry */
	dstword += SIGN;

    if (tmpword)			/* roll out to carry */
	SET_CC_C();
    else
	CLR_CC_C();

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CHG_CC_V_XOR_C_N();

    store_dst_2();
}

/* sbc() - Subtract Carry Instruction. */
void 
sbc()
{
    load_dst();

    if (dstword == MNI)
	SET_CC_V();
    else
	CLR_CC_V();

    if (CC_C) {		/* do if carry is set */
	if (dstword)
	    CLR_CC_C();
	else
	    SET_CC_C();
	--dstword;			/* subtract carry */
    } else {
	CLR_CC_C();
    }

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);

    store_dst_2();
}

/* swabi() - Swap Bytes Instruction. */
void 
swabi()
{
    u_int16_t data2;
    u_int16_t data3;

    load_dst();

    data2 = (dstword << 8) & 0xff00;
    data3 = (dstword >> 8) & 0x00ff;
    dstword = data2 + data3;

    CLR_CC_ALL();
    CHGB_CC_N(data3);		/* cool, negative and zero */
    CHGB_CC_Z(data3);		/* checks done on low byte only */

    store_dst_2();
}

/* sxt() - Sign Extend Instruction. */
void 
sxt()
{
    if (CC_N) {
	dstword = NEG_1;
	CLR_CC_Z();
    } else {
	dstword = 0;
	SET_CC_Z();
    }
    CLR_CC_V();

    store_dst();
}

/* tst() - Test Instruction. */
void 
tst()
{
    load_dst();

    CLR_CC_ALL();
    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
}

/* tstb() - Test Byte Instruction. */
void 
tstb()
{
    loadb_dst();

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);
    CLR_CC_V();
    CLR_CC_C();

}

/* aslb() - Arithmetic Shift Left Byte Instruction. */
void 
aslb()
{
    loadb_dst();

    if (dstbyte & SIGN_B)
	SET_CC_C();
    else
	CLR_CC_C();

    dstbyte <<= 1;

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);
    CHG_CC_V_XOR_C_N();

    storeb_dst_2();
}

/* asrb() - Arithmetic Shift Right Byte Instruction. */
void 
asrb()
{
    loadb_dst();

    if (dstbyte & LSBIT)
	SET_CC_C();
    else
	CLR_CC_C();

    dstbyte = (dstbyte >> 1) + (dstbyte & SIGN_B);    /* shift and replicate */

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);
    CHG_CC_V_XOR_C_N();

    storeb_dst_2();
}

/* clrb() - Clear Byte Instruction. */
void 
clrb()
{
    CLR_CC_ALL(); SET_CC_Z();
    srcbyte=0; storeb_dst();
}


/* comb() - Complement Byte Instruction. */
void 
comb()
{
    loadb_dst();

    dstbyte = ~dstbyte;

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);
    CLR_CC_V();
    SET_CC_C();

    storeb_dst_2();
}

/* decb() - Decrement Byte Instruction. */
void 
decb()
{
    loadb_dst();

    if (dstbyte == MNI_B)
	SET_CC_V();
    else
	CLR_CC_V();

    --dstbyte;

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);

    storeb_dst_2();
}

/* incb() - Increment Byte Instruction. */
void 
incb()
{
    loadb_dst();

    if (dstbyte == MPI_B)
	SET_CC_V();
    else
	CLR_CC_V();

    ++dstbyte;

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);

    storeb_dst_2();
}

/* negb() - Negate Byte Instruction. */
void 
negb()
{
    loadb_dst();

    dstbyte = (NEG_1_B - dstbyte) + 1;/* hope this is right */

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);

    if (dstbyte == MNI_B)
	SET_CC_V();
    else
	CLR_CC_V();

    if (dstbyte)
	SET_CC_C();
    else
	CLR_CC_C();

    storeb_dst_2();
}

/* rolb() - Rotate Left Byte Instruction. */
void 
rolb()
{
    loadb_dst();

    tmpbyte = dstbyte & SIGN_B; /* get top bit */
    dstbyte <<= 1;			/* shift */

    if (CC_C)		/* roll in carry */
	dstbyte = dstbyte + LSBIT;

    if (tmpbyte)			/* roll out to carry */
	SET_CC_C();
    else
	CLR_CC_C();

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);
    CHG_CC_V_XOR_C_N();

    storeb_dst_2();
}

/* rorb() - Rotate Right Byte Instruction. */
void 
rorb()
{
    loadb_dst();

    tmpbyte = dstbyte & LSBIT;	/* get low bit */
    dstbyte >>= 1;			/* shift */

    if (CC_C)		/* roll in carry */
	dstbyte += SIGN_B;

    if (tmpbyte)			/* roll out to carry */
	SET_CC_C();
    else
	CLR_CC_C();

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);
    CHG_CC_V_XOR_C_N();

    storeb_dst_2();
}

/* adcb() - Add Carry Byte Instruction. */
void 
adcb()
{
    loadb_dst();

    if (CC_C) {		/* do if carry is set */
	if (dstbyte == MPI_B)
	    SET_CC_V();
	else
	    CLR_CC_V();
	if (dstbyte == NEG_1_B)
	    SET_CC_C();
	else
	    CLR_CC_C();
	++dstbyte;			/* add the carry */
    } else {
	CLR_CC_V();
	CLR_CC_C();
    }

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);

    storeb_dst_2();
}

/* sbcb() - Subtract Carry Byte Instruction. */
void 
sbcb()
{
    loadb_dst();

    if (CC_C) {		/* do if carry is set */
	if (dstbyte)
	    CLR_CC_C();
	else
	    SET_CC_C();

	--dstbyte;			/* subtract carry */
    } else {
	CLR_CC_C();
    }

    if (dstbyte == MNI_B)
	SET_CC_V();
    else
	CLR_CC_V();

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);

    storeb_dst_2();
}
