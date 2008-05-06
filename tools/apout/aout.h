/* aout.h - parse and load the contents of a UNIX a.out file, for
 * several flavours of PDP-11 UNIX
 *
 * $Revision: 1.4 $
 * $Date: 2000/08/11 07:07:35 $
 */
#include <unistd.h>
#define EIGHT_K		 8192

/* UNIX magic numbers for the a.out header */
#define V1_NORMAL	0405	/* normal: 1st Edition, six words long */
#define ANY_NORMAL	0407	/* normal: V5,V6,V7,2.11BSD */
#define ANY_ROTEXT	0410	/* read-only text: V5,V6,V7,2.11BSD */
#define ANY_SPLITID	0411	/* seperated I&D: V5,V6,V7,2.11BSD */
#define BSD_OVERLAY	0430	/* 2.11BSD overlay, non-separate */
#define BSD_ROVERLAY	0431	/* 2.11BSD overlay, separate */
#define ANY_SCRIPT	020443	/* Shell script, i.e #! */
#define A68_MAGIC	0	/* Algol68 binaries have these magic nums */
#define A68_DATA       0107116	/* Algol68 binaries have these magic nums */

#define UNKNOWN_AOUT   034567	/* An unknown a.out header */

/* a.out header for nearly all UNIX flavours */
struct exec {
    u_int16_t a_magic;		/* magic number */
    u_int16_t a_text;		/* size of text segment */
    u_int16_t a_data;		/* size of initialised data */
    u_int16_t a_bss;		/* size of initialised bss */
    u_int16_t a_syms;		/* size of symbol table */
    u_int16_t a_entry;		/* entry point */
    u_int16_t a_unused;		/* unused */
    u_int16_t a_flag;		/* relocation info stripped */
				/* 16 bytes up to here */

				/* 2.11BSD overlay files have the following */
#define NOVL	15
     int16_t max_ovl;		/* maximum overlay size */
    u_int16_t ov_siz[NOVL];	/* size of the i'th overlay */
				/* Note that if the file isn't a 2.11BSD */
				/* overlay, we have to rewind to undo */
				/* the read of this section */
};

/* Because V5, V6, V7 and 2.11BSD share several magic numbers
 * in their a.out headers, we must distinguish them so as to
 * set up the correct emulated environment. This is done by
 * observing the differences in their crt0.s code: they all
 * differ at position 021
 */
#define a_magic2	ov_siz[0]
#define V2_M2		0177304		/* Doesn't apply to all, tho */
#define V6_M2		0010600
#define V7_M2		0016600
#define BSD_M2		0162706
