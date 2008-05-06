#include "aout.h"

/* This code borrowed from Apout */

extern int special_magic(u_int16_t *cptr);
extern void load_0407_symbols(FILE *zin, int offset, int size, int base);

int Binary;                  /* Type of binary this a.out is */
u_int8_t *ispace, *dspace;    /* Instruction and Data spaces */
static u_int8_t darray[PDP_MEM_SIZE], iarray[PDP_MEM_SIZE];

/* 2.11BSD allows up to 16 8K overlays in the 0430 and 0431 a.out types.
 * Each overlay is loaded at the first 8K `click' above the end of the
 * main text. The following structures hold the overlays from the current
 * a.out, if there are any. Missing overlays have size 0 and pointer NULL.
 */
static struct {
  u_int16_t size;
  u_int8_t *ovlay;
} ovlist[NOVL] = {
    {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL},
    {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL},
    {0, NULL}, {0, NULL}, {0, NULL}
};

static u_int8_t *ovbase;     /* Base address of 2.11BSD overlays */

/* Load the a.out header from the given file pointer, and return it.
 * Also return an integer describing which version of UNIX the a.out
 * belongs to. If errors on reading, return -1.
 */
int load_aout_header(FILE * zin, struct exec * E)
{
  char *cptr;

  				/* Read the a_magic value first */
				/* This makes it easier to deal with */
				/* parsing any script interpreter below */
  if (fread(E, sizeof(u_int16_t), 1, zin) != 1) return (-1);

  switch (E->a_magic) {
    case ANY_SCRIPT:		/* Shell script, return now */
      return (IS_UNKNOWN);
    case V1_NORMAL:
    case ANY_NORMAL:		/* These are recognised below */
    case ANY_ROTEXT:
    case ANY_SPLITID:
    case BSD_OVERLAY:
    case BSD_ROVERLAY:
    case A68_MAGIC:
      break;

    default:			/* Unrecognised binary, mark as such */
      E->a_magic = UNKNOWN_AOUT; return (IS_UNKNOWN);
  }

  				/* We can deal with this a.out, so */
  				/* read in the rest of the header */
  cptr = (char *) &(E->a_text);
  if (fread(cptr, sizeof(struct exec) - sizeof(u_int16_t), 1, zin) != 1)
    								return (-1);

  switch (E->a_magic) {
    case A68_MAGIC: if (E->a_data==A68_DATA) return(IS_A68);
		    else { E->a_magic = UNKNOWN_AOUT; return (IS_UNKNOWN); }
    case V1_NORMAL: return (IS_V1);
    case BSD_OVERLAY:
    case BSD_ROVERLAY: return (IS_211BSD);
    case ANY_NORMAL:
    case ANY_ROTEXT:
    case ANY_SPLITID:	/* Check crt0.o 2nd magic for V2/V6/V7/2.11BSD */
      if (E->a_magic2 == V2_M2) return (IS_V2);
      if (E->a_magic2 == V6_M2) return (IS_V6);
      if (E->a_magic2 == V7_M2) return (IS_V7);
      if (E->a_magic2 == BSD_M2) return (IS_211BSD);

                                /* Still no idea, use checksum to determine */
      return(special_magic((u_int16_t *) E));

    default:			/* Should never get here */
      E->a_magic = UNKNOWN_AOUT; return (IS_UNKNOWN);
  }
}


/* Load the named PDP-11 executable file into the disassembler's memory.
 * Returns 0 if ok, -1 if error.
 */
int load_a_out(const char *file, struct exec * e)
{				/* @globals errno,stdout,stderr; @ */
#define V12_MEMBASE 16384	/* Offset for V1/V2 binaries load */
  FILE *zin;
  u_int8_t *ibase, *dbase, *bbase;	/* Instruction, data, bss bases */
  u_int16_t size;
  int i,j;

  if ((zin = fopen(file, "r"))==NULL)	/* Open the file */
    return (-1);

  Binary = load_aout_header(zin, e);	/* Determine a.out & Unix type */

  if (e->a_magic == ANY_SCRIPT) {
    return (-1);
  }
  if (e->a_magic == UNKNOWN_AOUT) {
    return (-1);
  }

  switch (e->a_magic) {
    case V1_NORMAL:			/* V1 a.out binary looks like */
      e->a_bss = e->a_syms;		/* 0405			      */
      e->a_syms = e->a_data;		/* size of text		      */
      e->a_data = 0;			/* size of symbol table	      */
      					/* reloc bits		      */
      					/* size of data (i.e bss)     */
      					/* unused and zeroed	      */
      					/* We must rearrange fields   */
      					/* Move back to start of V1 header */
      if (fseek(zin, 0, SEEK_SET) != 0) {
	(void) fclose(zin); return (-1);
      }
      ispace = dspace = darray;
      ibase = &(ispace[V12_MEMBASE]);	/* Load & run the binary starting */
      dbase = &(ispace[e->a_text]);	/* at address 16384 (040000) */
      bbase = &(ispace[e->a_text + e->a_data]);
      e->a_entry = V12_MEMBASE + 12;	/* Add 12 to skip over a.out hdr */
      break;

    case A68_MAGIC:			/* Algol 68 image */
	if (fseek(zin, 0, SEEK_SET) != 0) {
	  (void) fclose(zin); return (-1);
	}
	e->a_text= e->ov_siz[0]+1;
	e->a_data= 0;
	e->a_bss= 0160000-e->a_text;
	e->a_entry= e->a_flag;
	ibase = ispace = dspace = darray;
	dbase= ibase;
	bbase= &(ispace[e->a_text+e->a_data]);
	break;
    case ANY_NORMAL:
      					/* Move back to end of V5/6/7 header */
      if (fseek(zin, 16, SEEK_SET) != 0) {
	(void) fclose(zin); return (-1);
      }
      ibase = ispace = dspace = darray;

      if (Binary == IS_V2) {
	ibase = &(ispace[V12_MEMBASE]);
        e->a_entry = V12_MEMBASE;
        dbase = &(ispace[e->a_text + V12_MEMBASE]);
        bbase = &(ispace[e->a_text + e->a_data + V12_MEMBASE]);
      } else {
        dbase = &(ispace[e->a_text]);
        bbase = &(ispace[e->a_text + e->a_data]);
      }

  				/* If there is a symbol table, load it */
      if (e->a_syms)
        load_0407_symbols(zin, 16 + e->a_text + e->a_data,e->a_syms,e->a_entry);
      break;
    case ANY_ROTEXT:
      					/* Move back to end of V5/6/7 header */
      if (fseek(zin, 16, SEEK_SET) != 0) {
	(void) fclose(zin); return (-1);
      }
      /* @fallthrough@ */
    case BSD_OVERLAY:
      				/* Round up text area to next 8K boundary */
      if (e->a_text % EIGHT_K) {
	size = EIGHT_K * (1 + e->a_text / EIGHT_K);
      } else size = e->a_text;
      				/* And the next 8K boundary if overlays! */
      if (e->a_magic == BSD_OVERLAY) {
	if (e->max_ovl % EIGHT_K) {
	  size += EIGHT_K * (1 + e->max_ovl / EIGHT_K);
	} else size += e->max_ovl;
      }
      ibase = ispace = dspace = darray;
      dbase = &(ispace[size]);
      bbase = &(ispace[size + e->a_data]);
      break;
    case ANY_SPLITID:
      					/* Move back to end of V5/6/7 header */
      if (fseek(zin, 16, SEEK_SET) != 0) {
	(void) fclose(zin); return (-1);
      }
      /* @fallthrough@ */
    case BSD_ROVERLAY:
      ibase = ispace = iarray;
      dbase = dspace = darray;
      bbase = &(dspace[e->a_data]);
      break;
    default:
      (void) fprintf(stderr, "Apout - unknown a.out format 0%o\n", e->a_magic);
      (void) fclose(zin); return (-1);
  }


  memset(darray, 0, PDP_MEM_SIZE);	/* Clear all memory */
  if (ispace != dspace) memset(iarray, 0, PDP_MEM_SIZE);

  					/* Now load the text into ibase */
  for (size = e->a_text; size;) {
    i = (int) fread(ibase, 1, (size_t) size, zin);
    if (i == -1) { (void) fclose(zin); return (i); }
    size -= i;
    ibase += i;
  }

  /* Now deal with any overlays */
  if (Binary == IS_211BSD)
    switch (e->a_magic) {
      case BSD_OVERLAY:
      case BSD_ROVERLAY:
				/* Round up text area to next 8K boundary */
	if (e->a_text % EIGHT_K) {
	  size = EIGHT_K * (1 + e->a_text / EIGHT_K);
	} else size = e->a_text;
	ovbase = &ispace[size];

	for (i = 0; i < NOVL; i++) {
	  if (e->ov_siz[i] == 0) {
	    ovlist[i].size = 0;
	    ovlist[i].ovlay = NULL;
	    continue;
	  }
	  				/* Create memory for the overlay */
	  ovlist[i].size = e->ov_siz[i];
	  if (ovlist[i].ovlay)
	    free(ovlist[i].ovlay);
	  ovlist[i].ovlay = (u_int8_t *) malloc(e->ov_siz[i]);
	  if (ovlist[i].ovlay == NULL) {
	    fprintf(stderr, "Apout - can't malloc overlay!\n");
	    exit(-1);
	  }
	  				/* Load the overlay into memory */
	  for (size = ovlist[i].size, ibase = ovlist[i].ovlay; size;) {
	    j = fread(ibase, 1, size, zin);
	    if (j == -1) {
	      fclose(zin); return (j);
	    }
	    size -= j;
	    ibase += j;
	  }
	}
    }

  				/* Now load the data into dbase */
  if (dbase)
    for (size = e->a_data; size;) {
      i = (int) fread(dbase, 1, (size_t) size, zin);
      if (i == -1) { (void) fclose(zin); return (i); }
      size -= i;
      dbase += i;
    }

  				/* Now clear the bss */
  if ((bbase != 0) && (e->a_bss != 0))
    memset(bbase, 0, (size_t) e->a_bss);


  (void) fclose(zin);
  return (0);
}
