/* cpu.c - this holds the main loop for the emulator, plus generic
 * functions to deal with exceptional instructions and events
 *
 * $Revision: 1.26 $
 * $Date: 2008/05/15 07:52:45 $
 */
#include "defines.h"
#include <unistd.h>

u_int8_t *ispace, *dspace;	/* Instruction and Data spaces */
u_int16_t dwrite_base=2;	/* Lowest addr where dspace writes can occur */

u_int16_t regs[8];		/* general registers */
u_int16_t ir;			/* current instruction register */
u_int16_t *adptr;		/* used in memory access macros */
u_int16_t ea_addr;		/* stored address for dest modifying insts */

int CC_N=0;			/* The processor status word is represented */
int CC_Z=0;			/* by these four values. On some */
int CC_V=0;			/* architectures, you may get a performance */
int CC_C=0;			/* increase by using shorts or bytes */

u_int16_t dstword;		/* These globals are used in the effective */
u_int16_t srcword;		/* address calculations, mainly to save */
u_int16_t tmpword;		/* parameter passing overheads in */
u_int8_t dstbyte;		/* function calls */
u_int8_t srcbyte;
u_int8_t tmpbyte;
struct our_siglist *Sighead=NULL;	/* List of pending signals */
struct our_siglist *Sigtail=NULL; 	/* List of pending signals */
void (*sigrunner)(void)= NULL; 		/* F'n that will run the signal */

#ifdef DEBUG
extern char *iname[1024];
extern char *iname0[64];	/* Name of each instruction */
extern char *iname1[64];
char *name;
#endif


/* Run until told to stop. */
void run() {
#ifdef DEBUG
    int i;

    if (trap_debug) {
	TrapDebug((dbg_file, "Just starting to run pid %d\n",(int)getpid()));
	TrapDebug((dbg_file, "Regs are           "));
	for (i=0;i<=PC;i++) TrapDebug((dbg_file, "%06o ",regs[i]));
	TrapDebug((dbg_file, "\n"));
    }
#endif

    while (1) {

	/* Fetch and execute the instruction. */

#ifdef DEBUG
	lli_word(regs[PC], ir);
	if (inst_debug) {
	   i= ir >> 6;
	   switch (i) {
		case 0: name= iname0[ir & 077]; break;
		case 2: name= iname1[ir & 077]; break;
		default: name= iname[i];
	   }
	   TrapDebug((dbg_file, "%06o %06o %4s ", regs[7], ir, name));
	   TrapDebug((dbg_file, "%06o %06o %06o %06o %06o %06o %06o   ",
		regs[0], regs[1], regs[2], regs[3],
		regs[4], regs[5], regs[6]));
	   TrapDebug((dbg_file, "NZVC1 %d%d%d%d\n",CC_N,CC_Z,CC_V,CC_C));
	   fflush(dbg_file);
	}
	regs[PC] += 2; itab[ir >> 6] ();
	if ((Sighead!=NULL) && (sigrunner!=NULL)) (void) (*sigrunner)();
#else
	/* When not debugging, we can manually unroll this inner loop */
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	lli_word(regs[PC], ir); regs[PC] += 2; itab[ir >> 6] ();
	if ((Sighead!=NULL) && (sigrunner!=NULL)) (void) (*sigrunner)();
#endif
    }
}

/* sim_init() - Initialize the cpu registers. */
void sim_init() {
    int x;

    for (x = 0; x < 8; ++x) { regs[x] = 0; }
    ir = 0; CLR_CC_ALL();
}

void bus_error(int signo)
{
    TrapDebug((dbg_file, "Apout - pid %d bus error at PC 0%06o\n",
					(int)getpid(), regs[PC]));
    TrapDebug((dbg_file, "%06o  ", ir));
    TrapDebug((dbg_file, "%o %o %o %o %o %o %o %o  ",
	 regs[0], regs[1], regs[2], regs[3],
	 regs[4], regs[5], regs[6], regs[7]));
    TrapDebug((dbg_file, "NZVC2 are %d%d%d%d\n",CC_N,CC_Z,CC_V,CC_C));
    exit(EXIT_FAILURE);
}

void seg_fault() {
    TrapDebug((dbg_file, "Apout - pid %d segmentation fault at PC 0%06o\n",
					(int)getpid(), regs[PC]));
    TrapDebug((dbg_file, "%06o  ", ir));
    TrapDebug((dbg_file, "%o %o %o %o %o %o %o %o  ",
	 regs[0], regs[1], regs[2], regs[3],
	 regs[4], regs[5], regs[6], regs[7]));
    TrapDebug((dbg_file, "NZVC3 are %d%d%d%d\n",CC_N,CC_Z,CC_V,CC_C));
    exit(EXIT_FAILURE);
}

void waiti() {
    TrapDebug((stderr, "Apout - pid %d waiti instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void halt() {
    TrapDebug((stderr, "Apout - pid %d halt instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void iot() {
    TrapDebug((stderr, "Apout - pid %d iot instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void emt() {
    TrapDebug((stderr, "Apout - pid %d emt instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void bpt() {
    TrapDebug((stderr, "Apout - pid %d bpt instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void illegal() {
    TrapDebug((stderr, "Apout - pid %d illegal instruction %o at PC 0%o\n",
					(int)getpid(),ir, regs[PC]));
    exit(EXIT_FAILURE);
}
void not_impl() {
    TrapDebug((stderr, "Apout - pid %d unimplemented instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void mark() {
    TrapDebug((stderr, "Apout - pid %d mark instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void mfpd() {
    TrapDebug((stderr, "Apout - pid %d mfpd instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void mtpd() {
    TrapDebug((stderr, "Apout - pid %d mtpd instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void trap() {
    TrapDebug((stderr, "Apout - pid %d trap instruction at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}
void bad_FP_reg() {
    TrapDebug((stderr, "Apout - pid %d bad FP register used at PC 0%o\n",
					(int)getpid(), regs[PC]));
    exit(EXIT_FAILURE);
}

/* This is the generic function which catches
 * a signal, and appends it to the queue.
 */
void sigcatcher(int sig)
{
 struct our_siglist *this;

 this= (struct our_siglist *)malloc(sizeof(struct our_siglist));
 if (this==NULL) return;

 TrapDebug((dbg_file, "Caught signal %d\n",sig));

 this->sig=sig; this->next=NULL;
 if (Sighead==NULL) { Sighead=Sigtail=this; }
 else { Sigtail->next= this; Sigtail=this; }
}
