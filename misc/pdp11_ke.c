/* pdp11_ke.c: 

   Copyright (c) 1993-2005, Robert M Supnik

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.

   Some of this code is lifted directly from Tim Shoppa's KE11A code in apout,
   Brad Parker <brad@heeltoe.com> 5/2008

   01-May-08    JBP     cloned from pdp11_pt.c
*/

#include "pdp11_defs.h"
//#define PT_DIS          0
extern int32 int_req[IPL_HLVL];

static int32 AC;
static int32 MQ;
static int32 SC;
static int32 SR;

DEVICE ke_dev;
t_stat ke_rd (int32 *data, int32 PA, int32 access);
t_stat ke_wr (int32 data, int32 PA, int32 access);
t_stat ke_svc (UNIT *uptr);
t_stat ke_reset (DEVICE *dptr);
t_stat ke_attach (UNIT *uptr, char *ptr);
t_stat ke_detach (UNIT *uptr);


DIB ke_dib = { IOBA_KE, IOLN_KE, &ke_rd, &ke_wr, 0 };

UNIT ke_unit = {
	UDATA (&ke_svc, UNIT_DISABLE, 0)
    };

REG ke_reg[] = {
      { ORDATA (KE_AC, AC, 16) },
      { ORDATA (KE_MQ, MQ, 16) },
      { ORDATA (KE_SC, SC, 16) },
      { ORDATA (KE_SR, SR, 16) },
    { NULL }
    };

MTAB ke_mod[] = {
    { MTAB_XTD|MTAB_VDV, 0, "ADDRESS", NULL,
      NULL, &show_addr, NULL },
    { MTAB_XTD|MTAB_VDV, 0, "VECTOR", NULL,
      NULL, &show_vec, NULL },
    { 0 }
    };

DEVICE ke_dev = {
    "KE", &ke_unit, ke_reg, ke_mod,
    1, 10, 31, 1, DEV_RDX, 8,
    NULL, NULL, &ke_reset,
    NULL, &ke_attach, &ke_detach,
    &ke_dib, DEV_DISABLE | /*PT_DIS |*/ DEV_UBUS | DEV_QBUS
    };

/* KE11A I/O address offsets 0177300 - 0177316 */

#define KE_DIV	000/* Divide */
#define KE_AC	002/* Accumulator */
#define KE_MQ	004/* MQ */
#define KE_MUL	006/* Multiply */
#define KE_SC	010/* Step counter */
#define KE_SR	011/* Status register */
#define KE_NOR	012/* Normalize */
#define KE_LSH	014/* Logical shift */
#define KE_ASH	016/* Arithmetic shift */

/* Stolen from Tim Shoppa's KE11A in apout */
void set_SR(void)
{
SR = SR & 0301;/* clear the result bits we can set here */
if (((MQ & 0100000) == 0) && (AC == 0)) SR = SR | 002;
if (((MQ & 0100000) == 0100000) && (AC == 0177777)) SR = SR | 002;

if ((AC == 0) && (MQ == 0)) SR = SR | 0004;
if (MQ == 0) SR = SR | 0010;
if (AC == 0) SR = SR | 0020;
if (AC == 0177777) SR = SR | 0040;
}

t_stat ke_rd (int32 *data, int32 PA, int32 access)
{
switch (PA & 077) {                               /* decode PA<5:0> */

    case KE_AC:
        *data = AC;
        return SCPE_OK;
    case KE_MQ:
        *data = MQ;
        return SCPE_OK;
    case KE_NOR:
        *data = SC;
        return SCPE_OK;
    case KE_SC:
        set_SR();
        *data = (SR << 8) | SC;
        return SCPE_OK;
    case KE_SR:
        set_SR();
        *data = (SR << 8);
        return SCPE_OK;

    case KE_DIV:
    case KE_MUL:
    case KE_LSH:
    case KE_ASH:
        *data = 0;
        return SCPE_OK;
        }

return SCPE_NXM;                                        /* can't get here */
}

t_stat ke_wr (int32 data, int32 PA, int32 access)
{
  int32 divisor, quotient, remainder;
  int32 dividend, product;
  int32 oldMQ;

switch (PA & 077) {                               /* decode PA<5:0> */

    case KE_DIV:
      SC = 0;
      dividend = (AC << 16) | MQ;
      divisor = data;
      if (divisor >> 15) divisor = divisor | ~077777;
      quotient = dividend / divisor;
      MQ = quotient & 0177777;
      remainder = dividend % divisor;
      AC = remainder & 0177777;
      SR = SR & 076;
      if ((quotient > 32767) || (quotient < -32768)) {	/* did we overflow? */
	if (dividend < 0) SR = SR | 0100;
	else SR = SR | 0200;
      } else {
	if (quotient < 0) SR = SR | 0300;
      }
      return SCPE_OK;

    case KE_AC:
      AC = data;
      if ((access == WRITEB) & (data >> 7))
	AC = AC | 0177400;
      return SCPE_OK;

    case KE_AC + 1:
      printf("write to AC+1; data=%o", data);
      AC = (AC & 0377) | (data << 8);
      return SCPE_OK;

    case KE_MQ:
      MQ = data;
      if ((access == WRITEB) & (data >> 7)) MQ = MQ | 0177400;
      if (MQ >> 15) AC = 0177777;
      else AC = 0;
      return SCPE_OK;

    case KE_MQ + 1:
      printf("write to MQ+1; data=%o", data);
      MQ = (MQ & 0377) | (data << 8);
      if (MQ >> 15) AC = 0177777;
      else AC = 0;
      return SCPE_OK;

    case KE_MUL:
      SC = 0;
      if (data >> 15) data = data | ~077777;
      if (MQ >> 15) MQ = MQ | ~077777;
      product = MQ * data;
      MQ = product & 0177777;
      AC = (product >> 16) & 0177777;
      SR = SR & 076;
      if (AC >> 15) SR = SR | 0300;	/* set sign bit if necessary */
      return SCPE_OK;

    case KE_SC:
      if (access == WRITEB) return SCPE_OK;/* byte writes are no-ops */
      SR = (data >> 8) & 0177777;
      SC = data & 0000077;
      return SCPE_OK;

    case KE_SR:
      return SCPE_OK;			/* this is a No-op */

    case KE_NOR:			/* Normalize */
      MQ = (AC << 16) | MQ;		/* 32-bit number to normalize in MQ */
      for (SC = 0; SC < 31; SC++) {
	if (MQ == (0140000 << 16))
	  break;
	if ((((MQ >> 30) & 3) == 1) || (((MQ >> 30) & 3) == 2))
	  break;
	MQ = MQ << 1;
      }
      printf("SC = %o\r\n", SC);
      AC = (MQ >> 16) & 0177777;
      MQ = MQ & 0177777;
      return SCPE_OK;

    case KE_LSH:			/* Logical shift */
       MQ=(AC<<16)|MQ; 			/* Form a temporary 32-bit entity */
       oldMQ=MQ & 0x80000000;		/* Save the sign bit for later */
       SR=SR&0176; 			/* Clear overflow & carry bits */
       data=data & 077;			/* Convert data from 6-bit */
       if (data>31) {		
	 data=64-data;			/* Shift in a -ve direction */
         SR=SR|((MQ>>(data-1))&1);	/* Get the bit that went off the end */
         MQ=MQ>>data; 			/* and do the right shift */
       } else {				/* Else left shift */
	 if ((MQ<<(data-1))&0x80000000) SR|=1; /* Get the bit off the end */
	 MQ=MQ<<data;			/* and do the left shift */
       }
       oldMQ= oldMQ ^ MQ;		/* Any difference in sign bit? */
       if (oldMQ & 0x80000000) SR|=0200;/* Yes, set the overflow bit */
       AC=(MQ>>16)&0177777;		/* Save result in AC and MQ */
       MQ=MQ&0177777;
       set_SR();
       return SCPE_OK;

    case KE_ASH:			/* Arithmetic shift */
       MQ=(AC<<16)|MQ; 			/* Form a temporary 32-bit entity */
       oldMQ=MQ & 0x80000000;		/* Save the sign bit for later */
       SR=SR&0176; 			/* Clear overflow & carry bits */
       data=data & 077;			/* Convert data from 6-bit */
       if (data>31) {		
	 data=64-data;			/* Shift in a -ve direction */
	 divisor=1 << data;		/* Work out the dividing factor */
         SR=SR|((MQ>>(data-1))&1);	/* Get the bit that went off the end */
         MQ=MQ/divisor; 		/* and do the right shift */
       } else {				/* Else left shift */
	 product=1 << data;		/* Work out the multiplying factor */
	 if ((MQ<<(data-1))&0x80000000) SR|=1; /* Get the bit off the end */
	 MQ=MQ*product;			/* and do the left shift */
       }
       oldMQ= oldMQ ^ MQ;		/* Any difference in sign bit? */
       if (oldMQ & 0x80000000) SR|=0200;/* Yes, set the overflow bit */
       AC=(MQ>>16)&0177777;		/* Save result in AC and MQ */
       MQ=MQ&0177777;
       set_SR();
       return SCPE_OK;

    }                                               /* end switch PA */

return SCPE_NXM;                                        /* can't get here */
}

/* service */

t_stat ke_svc (UNIT *uptr)
{
return SCPE_OK;
}

/* support routines */

t_stat ke_reset (DEVICE *dptr)
{
ke_unit.buf = 0;
sim_cancel (&ke_unit);                                 /* deactivate unit */
return SCPE_OK;
}

t_stat ke_attach (UNIT *uptr, char *cptr)
{
t_stat reason;
reason = attach_unit (uptr, cptr);
return reason;
}

t_stat ke_detach (UNIT *uptr)
{
return detach_unit (uptr);
}
