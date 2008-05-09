/* defines.h	- Definitions of things needed in all C files
 *
 * $Revision: 2.74 $
 * $Date: 2008/05/09 14:04:41 $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

/* Defines for ifdef'd code -- define them in the Makefile */

/* #define DEBUG			adds in debugging code */
/* #define ZERO_MEMORY			zeros all of process memory before
					it starts to run */
/* #define NATIVES			allows native binaries and PDP-11
					binaries in the filespace */
/* #define EMU211			add 2.11BSD emulation */
/* #define EMUV1			add 1st Edition emulation */
/* #define INLINE inline		inlines some functions (needs gcc) */
					

/* Optimisation defines */
#ifndef INLINE
# define INLINE
#endif

/* Special defines to enable/disable certain
 * functionality. These are added as required
 * to port to new platforms. Please send in new
 * defines, please!
 */

#if defined(__FreeBSD__) && __FreeBSD__ < 3
# define NO_GETPGID
#endif

#ifdef __FreeBSD__
# define Reboot(x) reboot(x)
#endif

#ifdef __linux__
# define NO_CHFLAGS
# define NO_STFLAGS
# define NO_GETPGID
# define NEED_MAP_FCNTL
# define SIGEMT 0
# ifndef SIGSYS
#  define SIGSYS 0
# endif
# define OXTABS XTABS
# define VDSUSP VSUSP		/* I don't think these are equivalent */
# define O_SHLOCK 0
# define O_EXLOCK 0
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
# define Reboot(x) reboot(x,NULL)
#endif

#ifndef Reboot
# define Reboot(x) exit(0)
#endif

#if !defined(__FreeBSD__) && !defined(__NetBSD__) && \
    !defined(__OpenBSD__) && !defined(__linux__) && !defined(__APPLE__)
# define NEED_INT_N
#endif

/* Type definitions for PDP data types. You may need to
 * define NEED_INT_N if your system  doesn't provide the
 * types defined below. If you do this, the best way is
 * to add some #if .. #define .. #endif lines above,
 * rather then modifying the ones below. If you make
 * changes to the #if's above, then I would  be very
 * happy to include them.
 *
 * Warren Toomey: wkt@tuhs.org
 */

#ifdef NEED_INT_N
typedef char int8_t;
typedef short int16_t;
typedef long int32_t;
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned long u_int32_t;
#endif

/* Macro defines for debug output, makes
 * the code look somewhat cleaner
 */

#ifdef DEBUG
# define TrapDebug(x) if (trap_debug) (void)fprintf x
# define InstDebug(x) if (inst_debug) (void)fprintf x
# define JsrDebug(x)  if (jsr_debug)  (void)fprintf x
# define FpDebug(x)   if (fp_debug)   (void)fprintf x
#else
# define TrapDebug(x)
# define InstDebug(x)
# define JsrDebug(x)
# define FpDebug(x)
#endif

				/* Defines for -DSTREAM_BUFFERING */
#define NFILE   40		/* Number of file pointers we can buffer */
#define ValidFD(x) ((x>=0) && (x<NFILE))
				/* Used for opening on directories */
#define TMP_PLATE       "/usr/tmp/apout_dir.XXXXXX"


/* Set up prototype macro for 
 * both K&R and ANSI C platforms
 */
#ifdef __STDC__
#define P(s) s
#else
#define P(s) ()
#endif

typedef void (*_itab) P((void));
extern _itab itab[];		/* Instruction Table for Fast Decode. */

typedef float  FLOAT;		/* For now, we use floats to do FP */

/* PDP processor defines. */

#define SP	6		/* stack pointer */
#define PC	7		/* program counter */
#define CC_NBIT 010		/* Positions of the flags in the */
#define CC_ZBIT 04		/* processor status word */
#define CC_VBIT 02
#define CC_CBIT 01

#define PDP_MEM_SIZE	65536	/* Size of inst-space and data-space */
#define MAX_ARGS	200	/* Max cmd-line args per process */


/* Global variables. */

extern u_int16_t regs[8];	/* general registers */
extern u_int16_t ir;		/* current instruction register */
extern int CC_N;		/* The processor status word is represented */
extern int CC_Z;		/* by these four values. On some */
extern int CC_V;		/* architectures, you may get a performance */
extern int CC_C;		/* increase by changing the size of the vars */

extern FLOAT  fregs[8];		/* FP registers */
extern int FPC;			/* FP Status flags */
extern int FPZ;
extern int FPN;
extern int FPV;
extern int FPMODE;		/* 0 = float, 1 = doubles */
extern int INTMODE;		/* 0 = integers, 1 = longs */

extern u_int16_t ea_addr;	/* stored address for dest modifying insts */
extern u_int8_t *ispace, *dspace;
extern u_int16_t dwrite_base;	/* Lowest addr where dspace writes can occur */

extern u_int16_t dstword;	/* These globals are used in the effective */
extern u_int16_t srcword;	/* address calculations, mainly to save */
extern u_int16_t tmpword;	/* parameter passing overheads in */
extern u_int8_t dstbyte;	/* function calls */
extern u_int8_t srcbyte;
extern u_int8_t tmpbyte;

				/* The following array holds the FILE pointers
				 * that correspond to open file descriptors.
				 * Only fds which are not ttys have
				 * FILE * pointers
				 */
extern FILE *stream[NFILE];
extern char *streammode[NFILE];

extern int sig_arrived;		/* Indicates if a signal has arrived */
extern int Argc, Envc;		/* Arguments passed to new process */
extern char *Argv[MAX_ARGS], *Envp[MAX_ARGS];
extern int Binary;		/* Type of binary this a.out is. One of: */
#define IS_UNKNOWN	0
#define IS_V1		1
#define IS_V2		2
#define IS_V3		3
#define IS_V4		4
#define IS_V5		5
#define IS_V6		6
#define IS_V7		7
#define IS_A68		68
#define IS_29BSD	29
#define IS_211BSD	211

				/* 2.11BSD overlay stuff */
extern u_int32_t ov_changes;	/* Number of overlay changes */
extern u_int8_t current_ov;	/* Current overlay number */

#ifdef DEBUG
				/* Debugging flags */
extern int inst_debug,		/* Print a line before each instruction */
    trap_debug,			/* Print details of each trap */
    jsr_debug,			/* Print out each jsr */
    fp_debug;			/* Print out each floating-point instruction */
extern FILE *dbg_file;		/* Debugging output file */
extern char *progname;		/* The program's name - used in debugging */
#endif

				/* We keep a list of signals that are pending */
struct our_siglist {
        int sig;		/* Number of the signal */
        struct our_siglist *next;
};
extern struct our_siglist *Sighead;	/* Head of the list */
extern struct our_siglist *Sigtail;	/* Tail of the list */

/* Inline defines. */

#define CLR_CC_V()	CC_V=0
#define CLR_CC_C()	CC_C=0
#define CLR_CC_Z()	CC_Z=0
#define CLR_CC_N()	CC_N=0
#define CLR_CC_ALL()	CC_V=CC_C=CC_Z=CC_N=0

#define SET_CC_V()	CC_V=1
#define SET_CC_C()	CC_C=1
#define SET_CC_Z()	CC_Z=1
#define SET_CC_N()	CC_N=1

#define SRC_MODE	(( ir & 07000 ) >> 9 )
#define SRC_REG		(( ir & 0700 ) >> 6 )
#define DST_MODE	(( ir & 070 ) >> 3 )
#define DST_REG		( ir & 07 )

#define LSBIT	1		/* least significant bit */

#define MPI	0077777		/* most positive integer */
#define MNI	0100000		/* most negative integer */
#define NEG_1	0177777		/* negative one */
#define SIGN	0100000		/* sign bit */
#define CARRY	0200000		/* set if carry out */

#define MPI_B	0177		/* most positive integer (byte) */
#define MNI_B	0200		/* most negative integer (byte) */
#define NEG_1_B 0377		/* negative one (byte) */
#define SIGN_B	0200		/* sign bit (byte) */
#define CARRY_B 0400		/* set if carry out (byte) */

#define LOW16( data )	(u_int16_t)(( data ) & 0177777 )	/* mask the lower 16 bits */
#define LOW8( data )	(u_int8_t)(( data ) & 0377 )	/* mask the lower 8 bits */

#define CHG_CC_N( d )	if ((d) & SIGN ) \
					SET_CC_N(); \
				else \
					CLR_CC_N()

#define CHGB_CC_N( d )	if ((d) & SIGN_B ) \
				SET_CC_N(); \
			else \
				CLR_CC_N()

#define CHG_CC_Z( d )	if ( d ) \
					CLR_CC_Z(); \
				else \
					SET_CC_Z()

#define CHGB_CC_Z( d )	if ( LOW8( d )) \
				CLR_CC_Z(); \
			else \
				SET_CC_Z()

#define CHG_CC_C( d )	if ((d) & CARRY ) \
					SET_CC_C(); \
				else \
					CLR_CC_C()

#define CHG_CC_IC( d )	if ((d) & CARRY ) \
					CLR_CC_C(); \
				else \
					SET_CC_C()

#define CHGB_CC_IC( d ) if ((d) & CARRY_B ) \
				CLR_CC_C(); \
			else \
				SET_CC_C()

#define CHG_CC_V( d1, d2, d3 )	\
				if ((( d1 & SIGN ) == ( d2 & SIGN )) \
				&& (( d1 & SIGN ) != ( d3 & SIGN ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHG_CC_VC( d1, d2, d3 ) \
				if ((( d1 & SIGN ) != ( d2 & SIGN )) \
				&& (( d2 & SIGN ) == ( d3 & SIGN ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHG_CC_VS( d1, d2, d3 ) \
				if ((( d1 & SIGN ) != ( d2 & SIGN )) \
				&& (( d1 & SIGN ) == ( d3 & SIGN ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHGB_CC_V( d1, d2, d3 ) \
				if ((( d1 & SIGN_B ) == ( d2 & SIGN_B )) \
				&& (( d1 & SIGN_B ) != ( d3 & SIGN_B ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHGB_CC_VC(d1,d2,d3)	\
				if ((( d1 & SIGN_B ) != ( d2 & SIGN_B )) \
				&& (( d2 & SIGN_B ) == ( d3 & SIGN_B ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHG_CC_V_XOR_C_N()	\
				if ((( CC_C ) && \
				   ( CC_N )) \
				|| ((!( CC_C )) && \
				   ( ! ( CC_N )))) \
					CLR_CC_V(); \
				else \
					SET_CC_V()


/* Macros to read and write loctions in
 * main memory.
 */

extern u_int16_t *adptr;

#define copylong(to,from) \
	buf = (char *) &(to); buf2 = (char *) &(from); \
	buf[0]=buf2[2]; buf[1]=buf2[3]; buf[2]=buf2[0]; buf[3]=buf2[1]

#ifndef EMUV1
/* lli_word() - Load a word from the given ispace logical address. */
#define lli_word(addr, word) \
	{ adptr= (u_int16_t *)&(ispace[addr]); word= *adptr; }

/* ll_word() - Load a word from the given logical address. */
#define ll_word(addr, word) \
	{ adptr= (u_int16_t *)&(dspace[addr]); word= *adptr; }

/* sl_word() - Store a word at the given logical address. */
#ifdef WRITEBASE
#define sl_word(addr, word) \
	{ if ((u_int16_t)addr < dwrite_base) seg_fault(); \
	  adptr= (u_int16_t *)&(dspace[addr]); *adptr= word; }
#else
#define sl_word(addr, word) \
	{ adptr= (u_int16_t *)&(dspace[addr]); *adptr= word; }
#endif

/* lli_byte() - Load a byte from the given logical ispace address. */
#define lli_byte(addr, byte) \
    byte = ispace[addr];

/* ll_byte() - Load a byte from the given logical address. */
#define ll_byte(addr, byte) \
    byte = dspace[addr];

/* sl_byte() - Store a byte at the given logical address. */
#ifdef WRITEBASE
#define sl_byte(addr, byte) \
	{ if (addr < dwrite_base) seg_fault(); \
	  dspace[addr]= byte; }
#else
#define sl_byte(addr, byte) \
	{ dspace[addr]= byte; }
#endif
#else
				/* These versions of the macros are required */
				/* because the KE11-A module is mapped into */
				/* a process' memory space in 1st Edition */
#define KE11LO	0177300
#define KE11HI	0177317

/* lli_word() - Load a word from the given ispace logical address. */
#define lli_word(addr, word) \
	{ if ((Binary<IS_V3) && (addr>=KE11LO) && (addr<=KE11HI)) {	\
		word= kell_word(addr);					\
	  } else { adptr= (u_int16_t *)&(ispace[addr]); word= *adptr; }	\
	}

/* ll_word() - Load a word from the given logical address. */
#define ll_word(addr, word) \
	{ if ((Binary<IS_V3) && (addr>=KE11LO) && (addr<=KE11HI)) {	\
		word= kell_word(addr);					\
	  } else { adptr= (u_int16_t *)&(dspace[addr]); word= *adptr; }	\
	}

/* sl_word() - Store a word at the given logical address. */
#ifdef WRITEBASE
#define sl_word(addr, word) \
	{ if ((u_int16_t)addr < dwrite_base) seg_fault(); \
	  if ((Binary<IS_V3) && (addr>=KE11LO) && (addr<=KE11HI)) {	\
		kesl_word(addr, word);					\
	  } else { adptr= (u_int16_t *)&(dspace[addr]); *adptr= word; }	\
	}
#else
#define sl_word(addr, word) \
	{ if ((Binary<IS_V3) && (addr>=KE11LO) && (addr<=KE11HI)) {	\
		kesl_word(addr, word);					\
	  } else { adptr= (u_int16_t *)&(dspace[addr]); *adptr= word; }	\
	}
#endif

/* lli_byte() - Load a byte from the given logical ispace address. */
#define lli_byte(addr, byte) \
    byte = ispace[addr];

/* ll_byte() - Load a byte from the given logical address. */
#define ll_byte(addr, byte) \
	{ if ((Binary<IS_V3) && (addr>=KE11LO) && (addr<=KE11HI)) {	\
		byte= kell_byte(addr);					\
    	  } else byte = dspace[addr];					\
	}

/* sl_byte() - Store a byte at the given logical address. */
#ifdef WRITEBASE
#define sl_byte(addr, byte) \
	{ if (addr < dwrite_base) seg_fault(); \
	  if ((Binary<IS_V3) && (addr>=KE11LO) && (addr<=KE11HI)) {	\
		kesl_byte(addr, byte);					\
	  } else dspace[addr]= byte; 					\
	}
#else
#define sl_byte(addr, byte) \
	{ if ((Binary<IS_V3) && (addr>=KE11LO) && (addr<=KE11HI)) {	\
		kesl_byte(addr, byte);					\
	  } else dspace[addr]= byte; 					\
	}
#endif
#endif
				



/* Function prototypes */

/* aout.c */
int load_a_out P((const char *file, const char *origpath, int want_env))
					/*@globals errno,stdout,stderr; @*/ ;
#ifdef EMU211
void do_bsd_overlay P((void));
#endif

/* branch.c */
void br P((void));
void blos P((void));
void bge P((void));
void blt P((void));
void ble P((void));
void bgt P((void));
void jmp P((void));
void jsr P((void));
void rts P((void));
void bne P((void));
void beq P((void));
void bpl P((void));
void bmi P((void));
void bhi P((void));
void bvc P((void));
void bvs P((void));
void bcc P((void));
void bcs P((void));
void scc P((void));
void ccc P((void));
void sob P((void));
void mfps P((void));
void mtps P((void));
void mfpi P((void));
void mtpi P((void));
void ash P((void));
void mul P((void));
void divide P((void));
void ashc P((void));
void xor P((void));

/* double.c */
void mov P((void));
void movsreg P((void));
void movsreg1 P((void));
void movsreg1pc P((void));
void cmp P((void));
void add P((void));
void sub P((void));
void bit P((void));
void bic P((void));
void bis P((void));
void movb P((void));
void cmpb P((void));
void bitb P((void));
void bicb P((void));
void bisb P((void));

/* cpu.c */
void run P((void));
void sim_init P((void));
void bus_error P((int));
void seg_fault P((void));
void waiti P((void));
void halt P((void));
void iot P((void));
void emt P((void));
void bpt P((void));
void fis P((void));
void illegal P((void));
void not_impl P((void));
void mark P((void));
void mfpd P((void));
void mtpd P((void));
void trap P((void));
void bad_FP_reg P((void));
void sigcatcher P((int sig));

/* ea.c */
void load_ea P((void));
void pop P((void));
void push P((void));
void loadb_dst P((void));
void loadb_src P((void));
void storeb_dst P((void));
void storeb_dst_2 P((void));
void loadp_dst P((void));
void storep_dst P((void));
void load_src P((void));
void store_dst P((void));
void load_dst P((void));
void store_dst_2 P((void));

/* fp.c */
void fpset P((void));
void ldf P((void));
void stf P((void));
void clrf P((void));
void addf P((void));
void subf P((void));
void negf P((void));
void absf P((void));
void mulf P((void));
void moddf P((void));
void divf P((void));
void cmpf P((void));
void tstf P((void));
void ldfps P((void));
void stfps P((void));
void lcdif P((void));
void stcfi P((void));
void stexp P((void));
void stcdf P((void));
void ldcdf P((void));
void stst P((void));
void ldexpp P((void));

/* itab.c */
void dositab0 P((void));
void dositab1 P((void));

/* main.c */
int main P((int argc, char **argv));
void usage P((void));
char * xlate_filename P((char *name));
void set_apout_root P((char *dirname));

/* magic.c */
int special_magic P((u_int16_t *cptr));

/* single.c */
void adc P((void));
void asl P((void));
void asr P((void));
void clr P((void));
void com P((void));
void dec P((void));
void inc P((void));
void neg P((void));
void rol P((void));
void ror P((void));
void sbc P((void));
void swabi P((void));
void sxt P((void));
void tst P((void));
void tstb P((void));
void aslb P((void));
void asrb P((void));
void clrb P((void));
void comb P((void));
void decb P((void));
void incb P((void));
void negb P((void));
void rolb P((void));
void rorb P((void));
void adcb P((void));
void sbcb P((void));

/* v6trap.c */
void v6trap P((void));

/* v7trap.c */
void v7trap P((void));

/* v1trap.c */
void v1trap P((void));

/* bsdtrap.c */
#ifdef EMU211
void bsdtrap P((void))			/*@globals errno,stdout,stderr; @*/ ;

/* bsd_ioctl.h */
int trap_ioctl P((void));

/* bsd_signal.c */
void set_bsdsig_dfl P((void));
int do_sigaction P((int sig, int a, int oa));
#endif

/* ke11a.c */
#ifdef EMUV1
int16_t kell_word P((u_int16_t addr));
void kesl_word P((u_int16_t addr, u_int16_t word));
int8_t kell_byte P((u_int16_t addr));
void kesl_byte P((u_int16_t addr, u_int8_t byte));
void set_SR P((void));
#endif

#undef P
