/* double.c - Double operand instructions.
 *
 * $Revision: 2.11 $
 * $Date: 1999/12/27 10:19:40 $
 */
#include "defines.h"

static u_int32_t templong;


/* mov() - Move Instruction.  Move operations with registers as the source
 * and/or destination have been inlined. */
void
mov()
{

    if (SRC_MODE) {
	load_src(); dstword=srcword;
    } else {
	dstword = regs[SRC_REG];
    }

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CLR_CC_V();

    if (DST_MODE) {
	store_dst();
    } else {
	regs[DST_REG] = dstword;
    }
}

/* movsreg(), movsreg1() and movsreg1pc() can all be replaced with
 * mov() above. I've broken them out in an attempt to improve
 * performance.
 */
void
movsreg()
{
    dstword = regs[SRC_REG];
    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CLR_CC_V();

    if (DST_MODE) {
	store_dst();
    } else {
	regs[DST_REG] = dstword;
    }
}

void
movsreg1()
{
    ll_word(regs[SRC_REG], dstword);
    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CLR_CC_V();

    if (DST_MODE) {
	store_dst();
    } else {
	regs[DST_REG] = dstword;
    }
}

void
movsreg1pc()
{
    lli_word(regs[PC], dstword)
    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CLR_CC_V();

    if (DST_MODE) {
	store_dst();
    } else {
	regs[DST_REG] = dstword;
    }
}



/* cmp() - Compare Instruction. */
void
cmp()
{
    load_src();
    load_dst();

    tmpword = ~dstword;
    templong = ((u_int32_t) srcword) + ((u_int32_t) (tmpword)) + 1;
    tmpword = LOW16(templong);

    CHG_CC_N(tmpword);
    CHG_CC_Z(tmpword);
    CHG_CC_VC(srcword, dstword, tmpword);	/* was CHG_CC_V */
    CHG_CC_IC(templong);
}


/* add() - Add Instruction. */
void
add()
{
    load_src();
    load_dst();

    templong = ((u_int32_t) srcword) + ((u_int32_t) dstword);
    tmpword = LOW16(templong);

    CHG_CC_N(tmpword);
    CHG_CC_Z(tmpword);
    CHG_CC_V(srcword, dstword, tmpword);
    CHG_CC_C(templong);

    dstword=tmpword; store_dst_2();
}

/* Subtract Instruction. */
void
sub()
{
    load_src();
    load_dst();

    tmpword = ~srcword;
    templong = ((u_int32_t) dstword) + ((u_int32_t) tmpword) + 1;
    tmpword = LOW16(templong);

    CHG_CC_N(tmpword);
    CHG_CC_Z(tmpword);
    CHG_CC_VS(srcword, dstword, tmpword);	/* was CHG_CC_V */
    CHG_CC_IC(templong);

    dstword=tmpword; store_dst_2();
}


/* bit() - Bit Test Instruction. */
void
bit()
{
    load_src();
    load_dst();

    dstword = srcword & dstword;

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CLR_CC_V();
}

/* bic() - Bit Clear Instruction. */
void
bic()
{
    load_src();
    load_dst();

    dstword = (~srcword) & dstword;
    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CLR_CC_V();

    store_dst_2();
}


/* bis() - Bit Set Instruction. */
void
bis()
{
    load_src();
    load_dst();

    dstword = srcword | dstword;

    CHG_CC_N(dstword);
    CHG_CC_Z(dstword);
    CLR_CC_V();

    store_dst_2();
}

/* movb() - Move Byte Instruction.  Move operations with registers as the
 * source and/or destination have been inlined. */
void
movb()
{
    if (SRC_MODE) {
	loadb_src();
    } else {
	srcbyte = LOW8(regs[SRC_REG]);
    }

    CHGB_CC_N(srcbyte);
    CHGB_CC_Z(srcbyte);
    CLR_CC_V();

    /* move byte to a register causes sign extension */

    if (DST_MODE) {
	storeb_dst();
    } else {
	if (srcbyte & SIGN_B)
	    regs[DST_REG] = (u_int16_t)0177400 + (u_int16_t)srcbyte;
	else
	    regs[DST_REG] = (u_int16_t)srcbyte;
    }
}

/* cmpb() - Compare Byte Instruction. */
void
cmpb()
{
    u_int8_t data3;

    loadb_src();
    loadb_dst();

    data3 = (u_int8_t)~dstbyte;
    tmpword = ((u_int16_t) srcbyte) + ((u_int16_t) (data3)) + 1;
    data3 = LOW8(tmpword);

    CHGB_CC_N(data3);
    CHGB_CC_Z(data3);
    CHGB_CC_VC(srcbyte, dstbyte, data3);
    CHGB_CC_IC(tmpword);
}


/* bitb() - Bit Test Byte Instruction. */
void
bitb()
{
    loadb_src();
    loadb_dst();

    dstbyte = srcbyte & dstbyte;

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);
    CLR_CC_V();
}

/* bicb() - Bit Clear Byte Instruction. */
void
bicb()
{
    loadb_src();
    loadb_dst();

    dstbyte = (u_int8_t)((~srcbyte) & dstbyte);

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);
    CLR_CC_V();

    storeb_dst_2();
}


/* bisb() - Bit Set Byte Instruction. */

void
bisb()
{
    loadb_src();
    loadb_dst();

    dstbyte = srcbyte | dstbyte;

    CHGB_CC_N(dstbyte);
    CHGB_CC_Z(dstbyte);
    CLR_CC_V();

    storeb_dst_2();
}
