/* This code borrowed from 2.11BSD adb */

#include "aout.h"

extern void add_symbol(int addr, int type, int size);
extern struct symbol *get_isym(int addr);
extern struct symbol *get_dsym(int addr);

extern u_int8_t *ispace, *dspace;	/* Instruction and Data spaces */
extern struct syscallinfo *systab;
extern int numsyscalls;

int doprint = 0;		/* Only print out if 1 */
int itype;			/* Global equal to p->itype */

#define NSP     0
#define ISYM    2
#define DSYM    7

/* instruction printing */

#define DOUBLE  0
#define DOUBLW  1
#define SINGLE  2
#define SINGLW  3
#define REVERS  4
#define BRANCH  5
#define NOADDR  6
#define DFAULT  7
#define TRAP    8
#define SYS     9
#define SOB     10
#define JMP     11
#define JSR     12

struct optab {
  int mask;
  int val;
  int itype;
  char *iname;
}   optab[] = {
  { 0107777, 0010000, DOUBLE, "mov" },
  { 0107777, 0020000, DOUBLE, "cmp" },
  { 0107777, 0030000, DOUBLE, "bit" },
  { 0107777, 0040000, DOUBLE, "bic" },
  { 0107777, 0050000, DOUBLE, "bis" },
  { 0007777, 0060000, DOUBLW, "add" },
  { 0007777, 0160000, DOUBLW, "sub" },
  { 0100077, 0005000, SINGLE, "clr" },
  { 0100077, 0005100, SINGLE, "com" },
  { 0100077, 0005200, SINGLE, "inc" },
  { 0100077, 0005300, SINGLE, "dec" },
  { 0100077, 0005400, SINGLE, "neg" },
  { 0100077, 0005500, SINGLE, "adc" },
  { 0100077, 0005600, SINGLE, "sbc" },
  { 0100077, 0005700, SINGLE, "tst" },
  { 0100077, 0006000, SINGLE, "ror" },
  { 0100077, 0006100, SINGLE, "rol" },
  { 0100077, 0006200, SINGLE, "asr" },
  { 0100077, 0006300, SINGLE, "asl" },
  { 0000077, 0000100, JMP, "jmp" },
  { 0000077, 0000300, SINGLE, "swab" },
  { 0000077, 0170100, SINGLW, "ldfps" },
  { 0000077, 0170200, SINGLW, "stfps" },
  { 0000077, 0170300, SINGLW, "stst" },
  { 0000077, 0170400, SINGLW, "clrf" },
  { 0000077, 0170500, SINGLW, "tstf" },
  { 0000077, 0170600, SINGLW, "absf" },
  { 0000077, 0170700, SINGLW, "negf" },
  { 0000077, 0006700, SINGLW, "sxt" },
  { 0000077, 0006600, SINGLW, "mtpi" },
  { 0000077, 0106600, SINGLW, "mtpd" },
  { 0000077, 0006500, SINGLW, "mfpi" },
  { 0000077, 0106500, SINGLW, "mfpd" },
  { 0000077, 0106700, SINGLW, "mfps" },
  { 0000077, 0106400, SINGLW, "mtps" },
  { 0000777, 0070000, REVERS, "mul" },
  { 0000777, 0071000, REVERS, "div" },
  { 0000777, 0072000, REVERS, "ash" },
  { 0000777, 0073000, REVERS, "ashc" },
  { 0377, 0000400, BRANCH, "br" },
  { 0377, 0001000, BRANCH, "bne" },
  { 0377, 0001400, BRANCH, "beq" },
  { 0377, 0002000, BRANCH, "bge" },
  { 0377, 0002400, BRANCH, "blt" },
  { 0377, 0003000, BRANCH, "bgt" },
  { 0377, 0003400, BRANCH, "ble" },
  { 0377, 0100000, BRANCH, "bpl" },
  { 0377, 0100400, BRANCH, "bmi" },
  { 0377, 0101000, BRANCH, "bhi" },
  { 0377, 0101400, BRANCH, "blos" },
  { 0377, 0102000, BRANCH, "bvc" },
  { 0377, 0102400, BRANCH, "bvs" },
  { 0377, 0103000, BRANCH, "bcc" },
  { 0377, 0103400, BRANCH, "bcs" },
  { 0000000, 0000000, NOADDR, "halt" },
  { 0000000, 0000001, NOADDR, "wait" },
  { 0000000, 0000002, NOADDR, "rti" },
  { 0000000, 0000003, NOADDR, "bpt" },
  { 0000000, 0000004, NOADDR, "iot" },
  { 0000000, 0000005, NOADDR, "reset" },
  { 0000000, 0000006, NOADDR, "rtt" },
  { 0377, 0171000, REVERS, "mulf" },
  { 0377, 0171400, REVERS, "modf" },
  { 0377, 0172000, REVERS, "addf" },
  { 0377, 0172400, REVERS, "movf" },
  { 0377, 0173000, REVERS, "subf" },
  { 0377, 0173400, REVERS, "cmpf" },
  { 0377, 0174000, DOUBLW, "movf" },
  { 0377, 0174400, REVERS, "divf" },
  { 0377, 0175000, DOUBLW, "movei" },
  { 0377, 0175400, DOUBLW, "movfi" },
  { 0377, 0176000, DOUBLW, "movfo" },
  { 0377, 0176400, REVERS, "movie" },
  { 0377, 0177000, REVERS, "movif" },
  { 0377, 0177400, REVERS, "movof" },
  { 0000000, 0170000, NOADDR, "cfcc" },
  { 0000000, 0170001, NOADDR, "setf" },
  { 0000000, 0170002, NOADDR, "seti" },
  { 0000000, 0170011, NOADDR, "setd" },
  { 0000000, 0170012, NOADDR, "setl" },
  { 0000000, 0000007, NOADDR, "mfpt" },
  { 0000077, 0007000, JMP, "csm" },
  { 0000077, 0007300, SINGLW, "wrtlck" },
  { 0000077, 0007200, SINGLW, "tstset" },
  { 0000777, 0004000, JSR, "jsr" },
  { 0000777, 0074000, DOUBLE, "xor" },
  { 0000007, 0000200, SINGLE, "rts" },
  { 0000017, 0000240, DFAULT, "cflg" },
  { 0000017, 0000260, DFAULT, "sflg" },
  { 0377, 0104000, TRAP, "emt" },
  { 0377, 0104400, SYS, "sys" },
  { 0000077, 0006400, TRAP, "mark" },
  { 0000777, 0077000, SOB, "sob" },
  { 0000007, 0000230, DFAULT, "spl" },
  { 0177777, 0000000, DFAULT, "<illegal op>" }
};


/* Heuristically determine what follows after a jsr r5,xxx.
 * Create a symbol for it, including the estimated size.
 * Return the length of the data in bytes.
 */
int guess_jsr_r5(int addr)
{
  int a,len;
  int istext=1;

  /* Try an ASCII string first. Give up if we find a
   * non-printable and non-NUL character.
   */
  for (a=addr,len=0;; a++,len++) {
    /* Found end of string which isn't zero length, stop now */
    if (len && (ispace[a]=='\0')) {
	len++; break;
    }

    /* If char is not ASCII and not tab/newline, it's not a string */
    if (!isprint(ispace[a]) && (ispace[a] != '\t')  && (ispace[a] != '\n')) {
	istext=0; break;
    }
  }

  /* If not a string, guess a single word as argument */
  if (!istext) {
    add_symbol(addr, SYM_JSRDATA, 2);
    return(2);
  } else {
    /* Ensure length is word-aligned */
    if ((addr+len)%2) len++;
    add_symbol(addr, SYM_JSRTEXT, len);
    return(len);
  }
}

void printquoted(char *str) 
{
  while(*str) {
    switch(*str) {
    case '\n': printf("\\n"); break;
    case '\r': printf("\\r"); break;
    case '\b': printf("\\b"); break;
    default: printf("%c", *str);
    }
    str++;
  }
  printf("\\0");
}

/*
 * Print out an operand. Return any increment to skip to reach the next
 * instruction.
 */
int paddr(char *str, int addr, int a, int lastr)
{
  char *regname[] = {"r0", "r1", "r2", "r3", "r4", "r5", "sp", "pc"};
  u_int16_t var;
  int r;
  char *rptr;

  r = a & 07;
  a &= 070;

  if (doprint)
    printf(str);
  if (r == 7 && a & 020) {
    int jsrr5_skip=0;		/* Amount to skip on a jsr r5,xxx */
    if (a & 010) {
      if (doprint)
	putchar('*');
    }
    if (a & 040) {

      var = (addr + 4) + ispace[addr + 2] + (ispace[addr + 3] << 8);
      if (doprint) {
        /* See if there is a label for that */
        struct symbol *s;
	if (itype==JSR) s= get_isym(var);
	else s= get_dsym(var);
        if (s == NULL)
          printf("0%o", var);
        else
          printf("%s", s->name);

	/* We've hit a jsr r5,... */
	if ((itype==JSR) && (lastr==5)) {
	  s= get_isym(addr+4);
	  if (s==NULL) {
	    printf("Weird, no SYM_JSR\n");
	  } else {
	    jsrr5_skip= s->size;
	    if (s->type==SYM_JSRTEXT) {
	      char *str= (char *)&ispace[addr+4];
	      printf("; <");
              printquoted(str);
              printf(">; .even");
	    } else {
  	       u_int16_t var2= ispace[addr + 4] + (ispace[addr + 5] << 8);
	      printf("; 0%o", var2);
	    }
	  }
	}
      } else {
	/* Register a function if this is a JSR, else data */
	if (itype==JSR) {
	  add_symbol(var, SYM_FUNCTION, 0);
	  if (lastr==5) {
	    jsrr5_skip= guess_jsr_r5(addr+4);
	  }
	}
	else add_symbol(var, SYM_DATA, 0);
      }
    } else {
      var = ispace[addr + 2] + (ispace[addr + 3] << 8);
      if (doprint)
        printf("$0%o", var);
    }
    return (2+jsrr5_skip);
  }

  rptr = regname[r];
  switch (a) {
    /* r */
  case 000:
    if (doprint)
      printf(rptr);
    return (0);

    /* (r) */
  case 010:
    if (doprint)
      printf("(%s)", rptr);
    return (0);

    /* *(r)+ */
  case 030:
    if (doprint)
      putchar('*');

    /* (r)+ */
  case 020:
    if (doprint)
      printf("(%s)+", rptr);
    return (0);

    /* *-(r) */
  case 050:
    if (doprint)
      putchar('*');

    /* -(r) */
  case 040:
    if (doprint)
      printf("-(%s)", rptr);
    return (0);

    /* *x(r) */
  case 070:
    if (doprint)
      putchar('*');

    /* x(r) */
  case 060:
    var = ispace[addr + 2] + (ispace[addr + 3] << 8);
    if (doprint)
      printf("0%o(%s)", var, rptr);
    return (2);
  }
  return (0);
}

/*
 * Deal with double operands. Return any increment to skip to reach the next
 * instruction.
 */
int doubl(int addr, int a, int b)
{
  int i = 0;
  int r= a & 07;		/* Get the register from the 1st operand */
  i += paddr("        ", addr, a, 0);
  i += paddr(",", addr + i, b, r);
  return (i);
}

void branch(char *str, int addr, int ins)
{
  if (doprint) printf(str);
  if (ins & 0200) {
    ins |= 0177400;
  }

  /* Determine the branch address */
  ins = (addr + (ins << 1) + 2) & 0177777;

  if (!doprint) {
    add_symbol(ins, SYM_BRANCH, 0);	/* Add a branch symbol for it */
    return;
  }

  /* Printing it, see if there is a label for that */
  struct symbol *s = get_isym(ins);
  if (s == NULL) {
    printf("0%o", ins);
    return;
  }

  /* Yes there is, print it out */
  printf("%s", s->name);
  if (s->type == SYM_BRANCH) {
    if (ins > addr) printf("f");
    else printf("b");
  }
}

/*
 * Given an address, disassemble and print out the instruction at that
 * address. Return any increment to skip to reach the next instruction.
 */
int printins(int addr)
{
  unsigned int type;
  int byte;
  int incr = 2;
  int syscall;
  struct optab *p;
  struct symbol *s;

  /* Print out any instruction symbol */
  if (doprint) {
    s = get_isym(addr);
    if (s)
      printf("%s:\n", s->name);
  }
  /* Get the instruction from the ispace array */
  int ins = ispace[addr] + (ispace[addr + 1] << 8);

  type = DSYM;
  for (p = optab;; p++) {
    if ((ins & ~p->mask) == p->val) {
      break;
    }
  }
  if (doprint)
    printf("\t%s", p->iname);
  byte = ins & 0100000;
  ins &= p->mask;

  itype= p->itype;
  switch (p->itype) {

  case JMP:
    type = ISYM;

  case SINGLE:
    if (byte) {
      if (doprint)
	putchar('b');
    }
  case SINGLW:
    incr += paddr("        ", addr, ins, 0);
    break;

  case REVERS:
    incr += doubl(addr, ins & 077, (ins >> 6) & 07);
    break;

  case JSR:
    type = ISYM;

  case DOUBLE:
    if (byte) {
      if (doprint)
	putchar('b');
    }
  case DOUBLW:
    incr += doubl(addr, ins >> 6, ins);

  case NOADDR:
    break;

  case SOB:
    incr += paddr("        ", addr, (ins >> 6) & 07, 0);
    branch(",", addr, -(ins & 077));
    break;

  case BRANCH:
    branch("        ", addr, ins);
    break;

  case SYS:
    if (ins < numsyscalls && systab[ins].name) {
      if (doprint)
	printf("        %s", systab[ins].name);
      /* Print any argument words following the syscall */
      int n;
      for (n = 1; n <= systab[ins].numwords; n++) {
	int b = 2 * n;
	syscall = ispace[addr + b] + (ispace[addr + b + 1] << 8);
	if (doprint)
	  printf("; 0%o", syscall);
      }
      /* Skip some number of words following the syscall */
      incr += 2 * systab[ins].numwords;
    } else if (doprint)
      printf("        %d", ins);
    break;

  case TRAP:
  case DFAULT:
  default:
    if (doprint)
      printf("        0%o", ins);
  }
  if (doprint)
    printf("\n");
  return (incr);
}
