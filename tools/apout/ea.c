/* ea.c - Calculate, load, and store using the proper effective address as
 * specified by the current instruction.  Also push and pop stack operations.
 *
 * $Revision: 2.17 $
 * $Date: 1999/09/17 05:11:10 $
 */
#include "defines.h"

void 
load_ea(void)
{
    u_int16_t indirect;

    switch (DST_MODE) {
    case 0:
	illegal();
	return;
    case 1:
	dstword = regs[DST_REG];
	return;
    case 2:
	dstword = regs[DST_REG];	/* this is wrong for 11/34 */
	regs[DST_REG] += 2;
	return;
    case 3:
	indirect = regs[DST_REG];	/* this is wrong for 11/34 */
	regs[DST_REG] += 2;
	lli_word(indirect, dstword);
	return;
    case 4:
	regs[DST_REG] -= 2;
	dstword = regs[DST_REG];
	return;
    case 5:
	regs[DST_REG] -= 2;
	indirect = regs[DST_REG];
	lli_word(indirect, dstword);
	return;
    case 6:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	dstword = regs[DST_REG] + indirect;
	return;
    case 7:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	indirect = regs[DST_REG] + indirect;
	    ll_word(indirect, dstword);
	return;
    }
    illegal();
}


INLINE void 
pop(void)
{
    ll_word(regs[SP], dstword);
    regs[SP] += 2;
}


INLINE void 
push(void)
{
    regs[SP] -= 2;
    sl_word(regs[SP], srcword);
}


void 
loadb_dst(void)
{
    u_int16_t addr, indirect;

    switch (DST_MODE) {
    case 0:
	dstbyte = (u_int8_t)(regs[DST_REG] & 0377);
	return;
    case 1:
	addr = regs[DST_REG];
	ea_addr = addr;
	if (DST_REG == PC) {
	    lli_byte(addr, dstbyte)
	} else {
	    ll_byte(addr, dstbyte);
	}
	return;
    case 2:
	addr = regs[DST_REG];
	ea_addr = addr;
	if (DST_REG == PC) {
	    lli_byte(addr, dstbyte)
	} else {
	    ll_byte(addr, dstbyte);
	}
	if (DST_REG >= 6)
	    regs[DST_REG] += 2;
	else
	    regs[DST_REG] += 1;
	return;
    case 3:
	indirect = regs[DST_REG];
	if (DST_REG == PC) {
	    lli_word(indirect, addr)
	} else {
	    ll_word(indirect, addr);
	}
	ea_addr = addr;
	ll_byte(addr, dstbyte);
	regs[DST_REG] += 2;
	return;
    case 4:
	if (DST_REG >= 6)
	    regs[DST_REG] -= 2;
	else
	    regs[DST_REG] -= 1;
	addr = regs[DST_REG];
	ea_addr = addr;
	ll_byte(addr, dstbyte);
	return;
    case 5:
	regs[DST_REG] -= 2;
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	ea_addr = addr;
	ll_byte(addr, dstbyte);
	return;
    case 6:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	addr = regs[DST_REG] + indirect;
	ea_addr = addr;
	ll_byte(addr, dstbyte);
	return;
    case 7:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	indirect = regs[DST_REG] + indirect;
	    ll_word(indirect, addr);
	ea_addr = addr;
	ll_byte(addr, dstbyte);
	return;
    }
    illegal();
}


void 
loadb_src(void)
{
    u_int16_t addr, indirect;

    switch (SRC_MODE) {
    case 0:
	srcbyte = (u_int8_t)(regs[SRC_REG] & 0377);
	return;
    case 1:
	addr = regs[SRC_REG];
	if (SRC_REG == PC) {
	    lli_byte(addr, srcbyte);
	} else {
	    ll_byte(addr, srcbyte);
	}
	return;
    case 2:
	addr = regs[SRC_REG];
	if (SRC_REG == PC) {
	    lli_byte(addr, srcbyte);
	} else {
	    ll_byte(addr, srcbyte);
	}
	if (SRC_REG >= 6)
	    regs[SRC_REG] += 2;
	else
	    regs[SRC_REG] += 1;
	return;
    case 3:
	indirect = regs[SRC_REG];
	if (SRC_REG == PC) {
	    lli_word(indirect, addr)
	} else {
	    ll_word(indirect, addr);
	}
	ll_byte(addr, srcbyte);
	regs[SRC_REG] += 2;
	return;
    case 4:
	if (SRC_REG >= 6)
	    regs[SRC_REG] -= 2;
	else
	    regs[SRC_REG] -= 1;
	addr = regs[SRC_REG];
	ll_byte(addr, srcbyte);
	return;
    case 5:
	regs[SRC_REG] -= 2;
	indirect = regs[SRC_REG];
	ll_word(indirect, addr);
	ll_byte(addr, srcbyte);
	return;
    case 6:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	addr = regs[SRC_REG] + indirect;
	ll_byte(addr, srcbyte);
	return;
    case 7:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	indirect = regs[SRC_REG] + indirect;
	    ll_word(indirect, addr);
	ll_byte(addr, srcbyte);
	return;
    }
    illegal();
}

void 
storeb_dst(void)
{
    u_int16_t addr, indirect;

    switch (DST_MODE) {
    case 0:
	regs[DST_REG]&= 0xff00;
	regs[DST_REG]|= srcbyte;
	return;
    case 1:
	addr = regs[DST_REG];
	sl_byte(addr, srcbyte);
	return;
    case 2:
	addr = regs[DST_REG];
	sl_byte(addr, srcbyte);
	if (DST_REG >= 6)
	    regs[DST_REG] += 2;
	else
	    regs[DST_REG] += 1;
	return;
    case 3:
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	sl_byte(addr, srcbyte);
	regs[DST_REG] += 2;
	return;
    case 4:
	if (DST_REG >= 6)		/* xyz */
	    regs[DST_REG] -= 2;
	else
	    regs[DST_REG] -= 1;
	addr = regs[DST_REG];
	sl_byte(addr, srcbyte);
	return;
    case 5:
	regs[DST_REG] -= 2;
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	sl_byte(addr, srcbyte);
	return;
    case 6:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	addr = regs[DST_REG] + indirect;
	sl_byte(addr, srcbyte);
	return;
    case 7:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	indirect = regs[DST_REG] + indirect;
	    ll_word(indirect, addr);
	sl_byte(addr, srcbyte);
	return;
    }
    illegal();
}


INLINE void 
storeb_dst_2(void)
{
    if (DST_MODE == 0) {
	regs[DST_REG]&= 0xff00;
	regs[DST_REG]|= dstbyte;
	return;
    }
    sl_byte(ea_addr, dstbyte);
}


void 
loadp_dst(void)
{
    u_int16_t addr, indirect;

    switch (DST_MODE) {
    case 0:
	srcword = regs[DST_REG];
	return;
    case 1:
	addr = regs[DST_REG];
	ll_word(addr, srcword);
	return;
    case 2:
	addr = regs[DST_REG];
	ll_word(addr, srcword);
	regs[DST_REG] += 2;
	return;
    case 3:
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	ll_word(addr, srcword);
	regs[DST_REG] += 2;
	return;
    case 4:
	regs[DST_REG] -= 2;
	addr = regs[DST_REG];
	ll_word(addr, srcword);
	return;
    case 5:
	regs[DST_REG] -= 2;
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	ll_word(addr, srcword);
	return;
    case 6:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	addr = regs[DST_REG] + indirect;
	if (DST_REG == PC)
	    lli_word(addr, srcword)
	else
	    ll_word(addr, srcword);
	return;
    case 7:
	not_impl();
    }
    illegal();
}


void 
storep_dst(void)
{
    u_int16_t addr, indirect;

    switch (DST_MODE) {
    case 0:
	regs[DST_REG] = dstword;
	return;
    case 1:
	addr = regs[DST_REG];
	sl_word(addr, dstword);
	return;
    case 2:
	addr = regs[DST_REG];
	sl_word(addr, dstword);
	regs[DST_REG] += 2;
	return;
    case 3:
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	sl_word(addr, dstword);
	regs[DST_REG] += 2;
	return;
    case 4:
	regs[DST_REG] -= 2;
	addr = regs[DST_REG];
	sl_word(addr, dstword);
	return;
    case 5:
	regs[DST_REG] -= 2;
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	sl_word(addr, dstword);
	return;
    case 6:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	addr = regs[DST_REG] + indirect;
	sl_word(addr, dstword);
	return;
    case 7:
	not_impl();
    }
    illegal();
}


void 
load_src(void)
{
    u_int16_t addr, indirect;

    switch (SRC_MODE) {
    case 0:
	srcword = regs[SRC_REG];
	return;
    case 1:
	addr = regs[SRC_REG];
	if (SRC_REG == PC) {
	    lli_word(addr, srcword)
	} else {
	    ll_word(addr, srcword);
	}
	return;
    case 2:
	addr = regs[SRC_REG];
	if (SRC_REG == PC) {
	    lli_word(addr, srcword)
	} else {
	    ll_word(addr, srcword);
	}
	regs[SRC_REG] += 2;
	return;
    case 3:
	indirect = regs[SRC_REG];
	if (SRC_REG == PC) {
	    lli_word(indirect, addr)
	} else {
	    ll_word(indirect, addr);
	}
	regs[SRC_REG] += 2;	/* is this right ? */
	ll_word(addr, srcword);
	return;
    case 4:
	regs[SRC_REG] -= 2;
	addr = regs[SRC_REG];
	ll_word(addr, srcword);
	return;
    case 5:
	regs[SRC_REG] -= 2;
	indirect = regs[SRC_REG];
	ll_word(indirect, addr);
	ll_word(addr, srcword);
	return;
    case 6:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	addr = regs[SRC_REG] + indirect;
	ll_word(addr, srcword);
	return;
    case 7:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	indirect = regs[SRC_REG] + indirect;
	    ll_word(indirect, addr);
	ll_word(addr, srcword);
	return;
    }
    illegal();
}


void 
store_dst(void)
{
    u_int16_t addr, indirect;

    switch (DST_MODE) {
    case 0:
	regs[DST_REG] = dstword;
	return;
    case 1:
	addr = regs[DST_REG];
	sl_word(addr, dstword);
	return;
    case 2:
	addr = regs[DST_REG];
	sl_word(addr, dstword);
	regs[DST_REG] += 2;
	return;
    case 3:
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	regs[DST_REG] += 2;	/* is this right ? */
	sl_word(addr, dstword);
	return;
    case 4:
	regs[DST_REG] -= 2;
	addr = regs[DST_REG];
	sl_word(addr, dstword);
	return;
    case 5:
	regs[DST_REG] -= 2;
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	sl_word(addr, dstword);
	return;
    case 6:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	addr = regs[DST_REG] + indirect;
	sl_word(addr, dstword);
	return;
    case 7:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	indirect = regs[DST_REG] + indirect;
	    ll_word(indirect, addr);
	sl_word(addr, dstword);
	return;
    }
    illegal();
}


void 
load_dst(void)
{
    u_int16_t addr, indirect;

    switch (DST_MODE) {
    case 0:
	dstword = regs[DST_REG];
	return;
    case 1:
	addr = regs[DST_REG];
	ea_addr = addr;
	if (DST_REG == PC) {
	    lli_word(addr, dstword)
	} else {
	    ll_word(addr, dstword);
	}
	return;
    case 2:
	addr = regs[DST_REG];
	ea_addr = addr;
	if (DST_REG == PC) {
	    lli_word(addr, dstword)
	} else {
	    ll_word(addr, dstword);
	}
	regs[DST_REG] += 2;
	return;
    case 3:
	indirect = regs[DST_REG];
	if (DST_REG == PC) {
	    lli_word(indirect, addr)
	} else {
	    ll_word(indirect, addr);
	}
	ea_addr = addr;
	ll_word(addr, dstword);
	regs[DST_REG] += 2;
	return;
    case 4:
	regs[DST_REG] -= 2;
	addr = regs[DST_REG];
	ea_addr = addr;
	ll_word(addr, dstword);
	return;
    case 5:
	regs[DST_REG] -= 2;
	indirect = regs[DST_REG];
	ll_word(indirect, addr);
	ea_addr = addr;
	ll_word(addr, dstword);
	return;
    case 6:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	addr = regs[DST_REG] + indirect;
	ea_addr = addr;
	ll_word(addr, dstword);
	return;
    case 7:
	lli_word(regs[PC], indirect);
	regs[PC] += 2;
	indirect = regs[DST_REG] + indirect;
	    ll_word(indirect, addr);
	ea_addr = addr;
	ll_word(addr, dstword);
	return;
    }
    illegal();
}


INLINE void 
store_dst_2(void)
{
    if (DST_MODE == 0) {
	regs[DST_REG] = dstword;
	return;
    }
    sl_word(ea_addr, dstword);
}
