/* ke11a.c - this holds the emulation of the PDP 11/20 extended
 * arithmetic element. We only need this for 1st Edition a.out support.
 * Code kindly borrowed from the eae support written by Tim Shoppa
 * (shoppa@trailing-edge.com) for Bob Supnik's PDP-11 emulator.
 *
 * $Revision: 1.7 $
 * $Date: 1999/12/28 03:57:31 $
 */
#ifdef EMUV1
#include "defines.h"
#include <unistd.h>

void eae_wr(u_int16_t data, u_int16_t PA, int32_t access);
void set_SR(void);

/* I/O dispatch routine, I/O addresses 177300 - 177316 */

#define eae_DIV 0177300		/* Divide */
#define eae_AC  0177302		/* Accumulator */
#define eae_MQ	0177304		/* MQ */
#define eae_MUL	0177306		/* Multiply */
#define eae_SC  0177310		/* Step counter */
#define eae_SR  0177311		/* Status register */
#define eae_NOR 0177312		/* Normalize */
#define eae_LSH 0177314		/* Logical shift */
#define eae_ASH 0177316		/* Arithmetic shift */
#define WRITEB	1		/* Write of a byte */
#define WRITEW	2		/* Write of a word */

/* The MQ, AC, SC, and SR registers specify the state of the EAE */
/* Here we define them as int32's, though in real life the MQ and AC */
/* are 16 bits and the SC and SR are 8 bits */

int32_t MQ;				/* Multiply quotient */
int32_t AC;				/* Accumulator */
int32_t SC = 0;				/* Shift counter */
int32_t SR;				/* Status register */


/* Load a word from one of the KE11 registers */
int16_t kell_word(u_int16_t addr)
{
  int16_t data;
  int pid;

  switch (addr) {
    case eae_DIV:
      data = 0; break;
    case eae_MQ:
      data = MQ; break;
    case eae_AC:			/* high 16 bits of MQ */
      data = AC; break;
    case eae_SC:
      set_SR();
      data = (SR << 8) | SC; break;
    case eae_SR:
      set_SR();
      data = (SR << 8); break;
    case eae_NOR:
      data = SC; break;
    case eae_LSH:
    case eae_ASH:
    case eae_MUL:
      data = 0; break;
    default:
      pid = getpid();
      (void) fprintf(stderr, "Apout - pid %d unknown KE11 register 0%o\n",
		     pid, addr);
      exit(EXIT_FAILURE);
  }
  return data;
}

/* Load a byte from one of the KE11 registers */
int8_t kell_byte(u_int16_t addr)
{
  if (addr&1) printf("Hmm, KE11 access on 0%o\n",addr);
  return ((int8_t) kell_word(addr));
}

/* Save a word to one of the KE11 registers */
void kesl_word(u_int16_t addr, u_int16_t word)
{
  eae_wr(word, addr, WRITEW);
}

/* Save a byte to one of the KE11 registers */
void kesl_byte(u_int16_t addr, u_int8_t byte)
{
  eae_wr(byte, addr, WRITEB);
}

void eae_wr(u_int16_t data, u_int16_t PA, int32_t access)
{
  int32_t divisor, quotient, remainder;
  int32_t dividend, product;
  int32_t oldMQ;
  int pid;

  switch (PA) {
    case eae_DIV:
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
      return;
    case eae_AC:
      AC = data;
      if ((access == WRITEB) & (data >> 7))
	AC = AC | 0177400;
      return;
    case eae_AC + 1:
      printf("write to AC+1; data=%o", data);
      AC = (AC & 0377) | (data << 8);
      return;
    case eae_MQ:
      MQ = data;
      if ((access == WRITEB) & (data >> 7)) MQ = MQ | 0177400;
      if (MQ >> 15) AC = 0177777;
      else AC = 0;
      return;
    case eae_MQ + 1:
      printf("write to MQ+1; data=%o", data);
      MQ = (MQ & 0377) | (data << 8);
      if (MQ >> 15) AC = 0177777;
      else AC = 0;
      return;
    case eae_MUL:
      SC = 0;
      if (data >> 15) data = data | ~077777;
      if (MQ >> 15) MQ = MQ | ~077777;
      product = MQ * data;
      MQ = product & 0177777;
      AC = (product >> 16) & 0177777;
      SR = SR & 076;
      if (AC >> 15) SR = SR | 0300;	/* set sign bit if necessary */
      return;
    case eae_SC:
      if (access == WRITEB) return;	/* byte writes are no-ops */
      SR = (data >> 8) & 0177777;
      SC = data & 0000077;
      return;
    case eae_SR:
      return;				/* this is a No-op */
    case eae_NOR:			/* Normalize */
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
      return;
    case eae_LSH:			/* Logical shift */
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
       return;

    case eae_ASH:			/* Arithmetic shift */
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
       return;

    default:
      pid = getpid();
      (void) fprintf(stderr, "Apout - pid %d unknown KE11 register 0%o\n",
		     pid, PA);
      exit(EXIT_FAILURE);
  }
}

void set_SR(void)
{
  SR = SR & 0301;		/* clear the result bits we can set here */
  if (((MQ & 0100000) == 0) && (AC == 0)) SR = SR | 002;
  if (((MQ & 0100000) == 0100000) && (AC == 0177777)) SR = SR | 002;

  if ((AC == 0) && (MQ == 0)) SR = SR | 0004;
  if (MQ == 0) SR = SR | 0010;
  if (AC == 0) SR = SR | 0020;
  if (AC == 0177777) SR = SR | 0040;
}
#endif	/* EMUV1 */
