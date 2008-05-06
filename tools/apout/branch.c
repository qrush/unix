/* branch.c - Branch instructions, and instructions which are complex to do
 *
 * $Revision: 2.22 $
 * $Date: 1999/12/27 04:38:29 $
 */
#include "defines.h"

/* We use the following macro for the branch instructions below */

#define do_branch() \
    {	offset = LOW8(ir); \
	if (offset & SIGN_B) \
	    offset += 0177400; \
	regs[PC] += (offset * 2); \
    } \

static u_int16_t offset;

void bne() {
    if (CC_Z==0) do_branch();
}
void beq() {
    if (CC_Z==1) do_branch();
}
void bpl() {
    if (CC_N==0) do_branch();
}
void bmi() {
    if (CC_N==1) do_branch();
}
void bhi() {
    if ((CC_Z==0) && (CC_C==0)) do_branch();
}
void bvc() {
    if (CC_V==0) do_branch();
}
void bvs() {
    if (CC_V==1) do_branch();
}
void bcc() {
    if (CC_C==0) do_branch();
}
void bcs() {
    if (CC_C==1) do_branch();
}

/* br() - Branch Always. */
void br() {
    do_branch();
}


/* blos() - Branch Lower or Same Instruction. */
void blos() {
    if ((CC_C!=0) || (CC_Z!=0)) do_branch();
}

/* bge() - Branch Greater Than or Equal Instruction. */
void bge() {
    if ((CC_N ^ CC_V) == 0) do_branch();
}

/* blt() - Branch Less Than Instruction. */
void blt() {
    if ((CC_N ^ CC_V) == 1) do_branch();
}

/* ble() - Branch Less Than Or Equal Instruction. */
void ble() {
    if (((CC_N ^ CC_V) == 1) || ((CC_Z)!=0)) do_branch();
}

/* bgt() - Branch Greater Than Instruction. */
void bgt() {
    if (((CC_N ^ CC_V) == 0) && ((CC_Z) == 0)) do_branch();
}

/* jmp() - Jump Instruction. */
void jmp() {
    load_ea(); regs[PC]=dstword;
}

/* jsr() - Jump To Subroutine Instruction. */
void jsr() {
    load_ea();
    srcword=regs[SRC_REG]; push();
    regs[SRC_REG] = regs[PC];
    regs[PC] = dstword;
    JsrDebug((dbg_file, "jsr to 0%o\n", dstword));
}

/* rts() - Return From Subroutine Instruction. */
void rts() {
    regs[PC] = regs[DST_REG];
    pop(); regs[DST_REG] = dstword;
    JsrDebug((dbg_file, "rts to 0%o\n", regs[PC]));
}

void scc() {
    if (ir & CC_NBIT) CC_N=1;
    if (ir & CC_ZBIT) CC_Z=1;
    if (ir & CC_VBIT) CC_V=1;
    if (ir & CC_CBIT) CC_C=1;
}
void ccc() {
    if (ir & CC_NBIT) CC_N=0;
    if (ir & CC_ZBIT) CC_Z=0;
    if (ir & CC_VBIT) CC_V=0;
    if (ir & CC_CBIT) CC_C=0;
}

/* sob() - Subtract One and Branch Instruction. */
void sob() {
    regs[SRC_REG] -= 1;
    if (regs[SRC_REG]) {
	regs[PC] -= (ir & 077) * 2;
    }
}

/* mfps() - Move from Processor Status Instruction. */
void mfps() {
    srcbyte=(u_int8_t)0;
    if (CC_N) srcbyte|= CC_NBIT;
    if (CC_Z) srcbyte|= CC_ZBIT;
    if (CC_V) srcbyte|= CC_VBIT;
    if (CC_C) srcbyte|= CC_CBIT;

    CHGB_CC_N(srcbyte);
    CHGB_CC_Z(srcbyte);
    CLR_CC_V();

    if (DST_MODE) {
	storeb_dst();
    } else {
	if (srcbyte & SIGN_B) {
	    dstword = 0177400;
	} else {
	    dstword = 0;
	}
	dstword += (u_int16_t)srcbyte;
	store_dst();
    }
}

/* mtps() - Move to Processor Status Instruction. */
void mtps() {
    loadb_dst();
    if (dstbyte & CC_NBIT) CC_N=1;
    if (dstbyte & CC_ZBIT) CC_Z=1;
    if (dstbyte & CC_VBIT) CC_V=1;
    if (dstbyte & CC_CBIT) CC_C=1;
}

/* mfpi() - Move From Previous Instruction Space Instruction. */
void mfpi() {
    loadp_dst(); push();
}


/* mtpi() - To From Previous Instruction Space Instruction. */
void mtpi() {
     pop(); storep_dst();
}

/* ash() - Arithmetic Shift Instruction. */
void ash() {
    u_int16_t temp;
    u_int16_t old;
    u_int16_t sign;
    u_int16_t count;

    temp = regs[SRC_REG];
    load_dst();
    old = temp;

    if ((dstword & 077) == 0) {		/* no shift */
	CHG_CC_N(temp);
	CHG_CC_Z(temp);
	CLR_CC_V();
	return;
    }
    if (dstword & 040) {		/* right shift */
	count = 0100 - (dstword & 077);
	sign = temp & SIGN;
	while (count--) {
	    if (temp & LSBIT) {
		SET_CC_C();
	    } else {
		CLR_CC_C();
	    }
	    temp >>= 1;
	    temp += sign;
	}
    } else {				/* left shift */
	count = dstword & 037;
	while (count--) {
	    if (temp & SIGN) {
		SET_CC_C();
	    } else {
		CLR_CC_C();
	    }
	    temp <<= 1;
	}
    }

    CHG_CC_N(temp);
    CHG_CC_Z(temp);

    if ((old & SIGN) == (temp & SIGN)) {
	CLR_CC_V();
    } else {
	SET_CC_V();
    }
    regs[SRC_REG] = temp;
}


/* mul() and divide() - Multiply and Divide Instructions.  These work on
 * signed values, and we'll do the same.  This may not be portable. */

union s_u_word {
    u_int16_t u_word;
    short s_word;
};

union s_u_long {
    u_int32_t u_long;
    long s_long;
};

void mul() {
    union s_u_word data1;
    union s_u_word data2;
    union s_u_long tmp;

    data1.u_word = regs[SRC_REG];
    load_dst();
    data2.u_word=dstword;

    tmp.s_long = ((long) data1.s_word) * ((long) data2.s_word);

    regs[SRC_REG] = (u_int16_t)(tmp.u_long >> 16);
    regs[(SRC_REG) | 1] = (u_int16_t)(tmp.u_long & 0177777);

    CLR_CC_ALL();

    if (tmp.u_long == 0)
	SET_CC_Z();
    else
	CLR_CC_Z();

    if (tmp.u_long & 0x80000000)
	SET_CC_N();
    else
	CLR_CC_N();
}

void divide() {
    union s_u_long tmp;
    union s_u_long eql;
    union s_u_word data2;

    tmp.u_long = regs[SRC_REG];
    tmp.u_long = tmp.u_long << 16;
    tmp.u_long += regs[(SRC_REG) | 1];

    load_dst();
    data2.u_word=dstword;

    if (data2.u_word == 0) {
	SET_CC_C();
	SET_CC_V();
	return;
    } else {
	CLR_CC_C();
    }

    eql.s_long = tmp.s_long / data2.s_word;
    regs[SRC_REG] = (u_int16_t)eql.u_long & 0177777;

    if (eql.u_long == 0)
	SET_CC_Z();
    else
	CLR_CC_Z();

    if ((eql.s_long > 077777) || (eql.s_long < -0100000))
	SET_CC_V();
    else
	CLR_CC_V();

    if (eql.s_long < 0)
	SET_CC_N();
    else
	CLR_CC_N();

    eql.s_long = tmp.s_long % data2.s_word;
    regs[(SRC_REG) | 1] = (u_int16_t)eql.u_long & 0177777;
}

/* ashc() - Arithmetic Shift Combined Instruction. */
void ashc() {
    u_int32_t temp;
    u_int32_t old;
    u_int32_t sign;
    u_int16_t count;

    temp = regs[SRC_REG];
    temp <<= 16;
    temp += regs[(SRC_REG) | 1];
    old = temp;
    load_dst();

    if ((dstword & 077) == 0) { /* no shift */

	CLR_CC_V();

	if (temp & 0x80000000) {
	    SET_CC_N();
	} else {
	    CLR_CC_N();
	}

	if (temp) {
	    CLR_CC_Z();
	} else {
	    SET_CC_Z();
	}
	return;
    }
    if (dstword & 040) {		/* right shift */
	count = 0100 - (dstword & 077);
	sign = temp & 0x80000000;
	while (count--) {
	    if (temp & LSBIT) {
		SET_CC_C();
	    } else {
		CLR_CC_C();
	    }
	    temp >>= 1;
	    temp += sign;
	}
    } else {			/* left shift */
	count = dstword & 037;
	while (count--) {
	    if (temp & 0x80000000) {
		SET_CC_C();
	    } else {
		CLR_CC_C();
	    }
	    temp <<= 1;
	}
    }

    if (temp & 0x80000000)
	SET_CC_N();
    else
	CLR_CC_N();

    if (temp)
	CLR_CC_Z();
    else
	SET_CC_Z();

    if ((old & 0x80000000) == (temp & 0x80000000)) {
	CLR_CC_V();
    } else {
	SET_CC_V();
    }

    regs[SRC_REG] = (u_int16_t)(temp >> 16);
    regs[(SRC_REG) | 1] = LOW16(temp);
}

/* xor() - Exclusive Or Instruction */
void xor() {
    tmpword = regs[SRC_REG];

    load_dst();

    tmpword = tmpword ^ dstword;

    CHG_CC_N(tmpword);
    CHG_CC_Z(tmpword);
    CLR_CC_V();

    dstword=tmpword; store_dst_2();
}
