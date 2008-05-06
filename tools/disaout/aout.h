/*
 * aout.h - parse and load the contents of a UNIX a.out file, for several
 * flavours of PDP-11 UNIX
 * 
 * $Revision: 1.7 $ $Date: 2008/05/06 01:09:01 $
 */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define EIGHT_K		 8192
#define PDP_MEM_SIZE      65536	/* Size of inst-space and data-space */

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

/* Which version of UNIX this a.out comes from */
#define IS_UNKNOWN      0
#define IS_V1           1
#define IS_V2           2
#define IS_V3           3
#define IS_V4           4
#define IS_V5           5
#define IS_V6           6
#define IS_V7           7
#define IS_A68          68
#define IS_29BSD        29
#define IS_211BSD       211

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

/* Symbol table entries for 0407 binaries */
struct sym0407 {
  u_int8_t  name[8];
  u_int16_t type;
  u_int16_t addr;
};

#define ASYM_UNDEFINED	00
#define ASYM_ABSOLUTE	01
#define ASYM_TEXT	02
#define ASYM_DATA	03
#define ASYM_BSS	04
#define ASYM_UNDEFEXT	40
#define ASYM_ABSEXT	41
#define ASYM_TEXTEXT	42
#define ASYM_DATAEXT	43
#define ASYM_BSSDEXT	44

/*
 * Because V5, V6, V7 and 2.11BSD share several magic numbers in their a.out
 * headers, we must distinguish them so as to set up the correct emulated
 * environment. This is done by observing the differences in their crt0.s
 * code: they all differ at position 021
 */
#define a_magic2	ov_siz[0]
#define V2_M2		0177304	/* Doesn't apply to all, tho */
#define V6_M2		0010600
#define V7_M2		0016600
#define BSD_M2		0162706


/*
 * Some syscalls pass arguments in registers, some in words following the
 * trap instruction. For each Unix version, we keep an array of syscall
 * names, and the number of words following the trap
 */
struct syscallinfo {
  char *name;
  int numwords;
};

/*
 * We try to infer symbols based on branch addresses, jsr addresses etc. We
 * keep two lists, instruction symbols and data symbols. Each one is kept in
 * this struct.
 */
struct symbol {
  char *name;
  int type;
  int size;		/* # bytes, used by SYM_JSRTEXT and SYM_JSRDATA */
};

/* Symbol types */
#define SYM_BRANCH	0
#define SYM_FUNCTION	1
#define SYM_DATA	2
#define SYM_JSRTEXT	3	/* Ascii string following jsr r5,xxx */
#define SYM_JSRDATA	4	/* Binary data following jsr r5,xxx */
