/* aout.c - parse and load the contents of a UNIX a.out file, for
 * several flavours of PDP-11 UNIX
 *
 * $Revision: 1.51 $
 * $Date: 2008/05/19 13:42:39 $
 */
#include "defines.h"
#include "aout.h"

/* Array of 64K for data and instruction space */
static u_int8_t darray[PDP_MEM_SIZE], iarray[PDP_MEM_SIZE];

#ifdef EMU211
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

static u_int8_t *ovbase;	/* Base address of 2.11BSD overlays */
u_int32_t ov_changes = 0;	/* Number of overlay changes */
u_int8_t current_ov = 0;	/* Current overlay number */
#endif

/* Global array of pointers to arguments and environment. This
 * allows load_a_out() to modify it when dealing with shell
 * scripts, before calling set_arg_env()
 */
char *Argv[MAX_ARGS], *Envp[MAX_ARGS];
int Argc, Envc;
int Binary;			/* Type of binary this a.out is */

/* For programs without an environment, we set the environment statically.
 * Eventually there will be code to get some environment variables
 */
static char *default_envp[4] = {
  "PATH=/bin:/usr/bin:/usr/sbin:/usr/ucb:/usr/games:/usr/local/bin:.",
  "HOME=/",
  "TERM=vt100",
  "USER=root"
};
static int default_envc = 4;

/* Prototypes */
static void set_arg_env(int want_env);


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
  if (fread(cptr, 1, sizeof(struct exec) - sizeof(u_int16_t), zin)
		< 16 - sizeof (u_int16_t)) return (-1);

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


/* Read in the executable name and its arguments from the shell script,
 * and the re-call load_a_out to load in that binary. Returns 0 on
 * success, -1 on error. Input file is always closed by this routine.
 */
int load_script(const char *file, const char *origpath,FILE * zin, int want_env)
{
#define SCRIPT_LINESIZE 512	/* Max size of 1st line in script */
  char *script_line;
  char *script_arg[MAX_ARGS];
  int i, script_cnt = 0;
  char **ap;

  for (i=0;i<Argc;i++)
    TrapDebug((dbg_file, "In load_script Argv[%d] is %s\n", i, Argv[i]));
  				/* Get the first line of the file */
  if (((script_line = (char *) malloc(SCRIPT_LINESIZE)) == NULL) ||
      (fgets(script_line, SCRIPT_LINESIZE, zin) == NULL)) {
    (void) fprintf(stderr, "Apout - could not read 1st line of script\n");
    (void) fclose(zin);
    return (-1);
  }
  				/* Now break into separate words */
  for (ap = script_arg; (*ap = strsep(&script_line, " \t\n")) != NULL;)
    if (**ap != '\0') {
      ap++; script_cnt++;
      if (script_cnt >= MAX_ARGS) break;
    }
  if (fclose(zin) != 0) { free(script_line); return (-1); }

#ifdef DEBUG
  TrapDebug((dbg_file, "Script: extra args are is %d\n", script_cnt));
  if (trap_debug) {
    for (i = 0; i < script_cnt; i++)
      fprintf(dbg_file, " script_arg[%d] is %s\n", i, script_arg[i]);
  }
#endif

  				/* Ensure we have room to shift the args */
  if ((Argc + script_cnt) > MAX_ARGS) {
    (void) fprintf(stderr, "Apout - out of argv space in script\n");
    free(script_line); return (-1);
  }
  				/* Now shift the args up and insert new ones */
  for (i = Argc - 1; i != 0; i--) Argv[i + script_cnt] = Argv[i];
  for (i=0;i<Argc;i++)
    TrapDebug((dbg_file, "Part A load_script Argv[%d] is %s\n", i, Argv[i]));
  for (i = 0; i < script_cnt; i++) Argv[i] = script_arg[i];
  if (origpath!=NULL) Argv[i] = strdup(origpath);
  else Argv[i] = strdup(file);
  Argc += script_cnt;
  for (i=0;i<Argc;i++)
    TrapDebug((dbg_file, "Part B load_script Argv[%d] is %s\n", i, Argv[i]));

  file = xlate_filename(script_arg[0]);
  free(script_line);
  for (i=0;i<Argc;i++)
    TrapDebug((dbg_file, "Leaving load_script Argv[%d] is %s\n", i, Argv[i]));
  return (load_a_out(file, origpath, want_env));
}


/* Load the named PDP-11 executable file into the emulator's memory.
 * Returns 0 if ok, -1 if error. Also initialise the simulator and set
 * up the stack for the process with Argc, Argv, Envc, Envp.
 * origpath is the path to the executable as seen by the simulated
 * parent, or NULL if this is not known.
 */
int load_a_out(const char *file, const char *origpath, int want_env)
{				/* @globals errno,stdout,stderr; @ */
#define V12_MEMBASE 16384	/* Offset for V1/V2 binaries load */
  FILE *zin;
  struct exec e;
  u_int8_t *ibase, *dbase, *bbase;	/* Instruction, data, bss bases */
  u_int16_t size;
  int i;
#ifdef EMU211
  int j;
#endif
#ifdef RUN_V1_RAW
  struct stat stb;
#endif

  for (i=0;i<Argc;i++)
    TrapDebug((dbg_file, "In load_a_out Argv[%d] is %s\n", i, Argv[i]));

  (void) signal(SIGBUS, bus_error);	/* Catch all bus errors here */

  if ((zin = fopen(file, "r"))==NULL)	/* Open the file */
    return (-1);

  Binary = load_aout_header(zin, &e);	/* Determine a.out & Unix type */

  if (e.a_magic == ANY_SCRIPT) {	/* Shell script, run that */
    return (load_script(file, origpath, zin, want_env));
  }
#ifndef EMU211
  if (Binary == IS_211BSD) {
    (void) fprintf(stderr, "Apout not compiled to support 2.11BSD binaries\n");
    (void) fclose(zin); return (-1);
  }
#endif
#ifndef EMUV1
  if (Binary == IS_V1) {
    (void) fprintf(stderr,
		   "Apout not compiled to support 1st Edition binaries\n");
    (void) fclose(zin); return (-1);
  }
  if (Binary == IS_V2) {
    (void) fprintf(stderr,
		   "Apout not compiled to support 2nd Edition binaries\n");
    (void) fclose(zin); return (-1);
  }
#endif

#ifdef NATIVES
    					/* Executable was not recognised.
					 * Try to exec it as a native binary.
					 * If it fails, doesn't matter. If it
					 * succeeds, then great. This allows
					 * us to have mixed native and PDP-11
					 * binaries in the same filespace.
    					 */
  if (e.a_magic == UNKNOWN_AOUT) {
#ifdef DEBUG
    TrapDebug((dbg_file, "About to try native exec on %s\n", file));
    fflush(dbg_file);
#endif
    (void) fclose(zin);
    execv(file, Argv);		/* envp[] is the one Apout's main() got */
    TrapDebug((dbg_file, "Nope, didn't work\n"));
#endif

#ifdef RUN_V1_RAW
				/* Try to run it as a V1 raw binary */
#ifdef DEBUG
    TrapDebug((dbg_file, "About to try PDP-11 raw exec on %s\n", file));
    fflush(dbg_file);
#endif /* DEBUG */
    if ((zin = fopen(file, "r"))==NULL)        /* reopen the file */
      return (-1);
    e.a_magic = V1_RAW;
    Binary = IS_V1;
#else
     (void) fprintf(stderr, "Apout - unknown a.out file %s\n", file);
     return (-1);
#endif /* RUN_V1_RAW */
  }
  					/* Now we know what environment to
					 * create, set up the memory areas
					 * according to the magic numbers
					 */
#ifdef DEBUG
  switch(Binary) {
    case IS_A68: TrapDebug((dbg_file, "A68 binary\n")); break;
    case IS_V1:  TrapDebug((dbg_file, "V1 binary\n")); break;
    case IS_V2:  TrapDebug((dbg_file, "V2 binary\n")); break;
    case IS_V5:  TrapDebug((dbg_file, "V5 binary\n")); break;
    case IS_V6:  TrapDebug((dbg_file, "V6 binary\n")); break;
    case IS_V7:  TrapDebug((dbg_file, "V7 binary\n")); break;
    case IS_211BSD: TrapDebug((dbg_file, "2.11BSD binary\n")); break;
  }
#endif

  switch (e.a_magic) {
#ifdef RUN_V1_RAW
    case V1_RAW:
      if (fseek(zin, 0, SEEK_SET) != 0) {
       (void) fclose(zin); return (-1);
      }
      ispace = dspace = darray;
      ibase = &(ispace[V12_MEMBASE]);	/* Load & run the binary starting */
      dbase = ibase;			/* at address 16384 (040000) */
      dwrite_base = 0;
      e.a_entry = V12_MEMBASE;
      /* Reset the exec header fields to make loading code below
       * work properly
       */
      if (stat(file, &stb)) {
       fprintf(stderr, "Apout - cannot stat %s\n", file);
       return -1;
      }
      e.a_text = stb.st_size;
      bbase = &(ispace[V12_MEMBASE + e.a_text]);
      e.a_data = 0;
      break;
#endif
#ifdef EMUV1
    case V1_NORMAL:			/* V1 a.out binary looks like */
      e.a_bss = e.a_syms;		/* 0405			      */
      e.a_syms = e.a_data;		/* size of text		      */
      e.a_data = 0;			/* size of symbol table	      */
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
      dbase = &(ispace[e.a_text]);	/* at address 16384 (040000) */
      bbase = &(ispace[e.a_text + e.a_data]);
      dwrite_base = 0;
      e.a_entry = V12_MEMBASE;
      break;
#endif
    case A68_MAGIC:			/* Algol 68 image */
	if (fseek(zin, 0, SEEK_SET) != 0) {
	  (void) fclose(zin); return (-1);
	}
	e.a_text= e.ov_siz[0]+1;
	e.a_data= 0;
	e.a_bss= 0160000-e.a_text;
	e.a_entry= e.a_flag;
	ibase = ispace = dspace = darray;
	dbase= ibase;
	dwrite_base = 0;
	bbase= &(ispace[e.a_text+e.a_data]);
	break;
    case ANY_NORMAL:
      					/* Move back to end of V5/6/7 header */
      if (fseek(zin, 16, SEEK_SET) != 0) {
	(void) fclose(zin); return (-1);
      }
      ibase = ispace = dspace = darray;
#ifdef EMUV1
      if (Binary == IS_V2) {
	ibase = &(ispace[V12_MEMBASE]);
        e.a_entry = V12_MEMBASE;
        dbase = &(ispace[e.a_text + V12_MEMBASE]);
        bbase = &(ispace[e.a_text + e.a_data + V12_MEMBASE]);
      } else
#endif
      {
        dbase = &(ispace[e.a_text]);
        bbase = &(ispace[e.a_text + e.a_data]);
      }
      if ((Binary < IS_V7))
	   dwrite_base = 0;
      else dwrite_base = e.a_text;
      break;
    case ANY_ROTEXT:
      					/* Move back to end of V5/6/7 header */
      if (fseek(zin, 16, SEEK_SET) != 0) {
	(void) fclose(zin); return (-1);
      }
      /* @fallthrough@ */
    case BSD_OVERLAY:
      				/* Round up text area to next 8K boundary */
      if (e.a_text % EIGHT_K) {
	size = EIGHT_K * (1 + e.a_text / EIGHT_K);
      } else size = e.a_text;
      				/* And the next 8K boundary if overlays! */
      if (e.a_magic == BSD_OVERLAY) {
	if (e.max_ovl % EIGHT_K) {
	  size += EIGHT_K * (1 + e.max_ovl / EIGHT_K);
	} else size += e.max_ovl;
      }
      ibase = ispace = dspace = darray;
      dbase = &(ispace[size]);
      bbase = &(ispace[size + e.a_data]);
      dwrite_base = size;
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
      bbase = &(dspace[e.a_data]);
      
      					/* Try to stop null refs */
      if (Binary == IS_211BSD) dwrite_base = 0;
      else dwrite_base = 2;
      break;
    default:
      (void) fprintf(stderr, "Apout - unknown a.out format 0%o\n", e.a_magic);
      (void) fclose(zin); return (-1);
  }


  		/* Initialise the instruction table for our environment */
  switch (Binary) {
#ifdef EMU211
    case IS_211BSD:
      for (i = 548; i < 552; i++) itab[i] = bsdtrap;
      break;
#endif
#ifdef EMUV1
    case IS_V1:
    case IS_V2:
      for (i = 544; i < 548; i++) itab[i] = rts;
      for (i = 548; i < 552; i++) itab[i] = v1trap;
      break;
#endif
    case IS_A68:
      for (i = 544; i < 552; i++) itab[i] = v7trap;
      break;
    case IS_V5:
    case IS_V6:
    case IS_V7:
      for (i = 548; i < 552; i++) itab[i] = v7trap;
      break;
    default:
      fprintf(stderr, "Apout - unknown Unix version for %s\n", file);
      exit(EXIT_FAILURE);
  }

#ifdef ZERO_MEMORY
  memset(darray, 0, PDP_MEM_SIZE);	/* Clear all memory */
  if (ispace != dspace) memset(iarray, 0, PDP_MEM_SIZE);
#endif

  					/* Now load the text into ibase */
  for (size = e.a_text; size;) {
    i = (int) fread(ibase, 1, (size_t) size, zin);
    if (i == -1) { (void) fclose(zin); return (i); }
    size -= i;
    ibase += i;
  }

#ifdef EMU211
  /* Now deal with any overlays */
  if (Binary == IS_211BSD)
    switch (e.a_magic) {
      case BSD_OVERLAY:
      case BSD_ROVERLAY:
				/* Round up text area to next 8K boundary */
	if (e.a_text % EIGHT_K) {
	  size = EIGHT_K * (1 + e.a_text / EIGHT_K);
	} else size = e.a_text;
	ovbase = &ispace[size];

	for (i = 0; i < NOVL; i++) {
	  if (e.ov_siz[i] == 0) {
	    ovlist[i].size = 0;
	    ovlist[i].ovlay = NULL;
	    continue;
	  }
	  				/* Create memory for the overlay */
	  ovlist[i].size = e.ov_siz[i];
	  if (ovlist[i].ovlay)
	    free(ovlist[i].ovlay);
	  ovlist[i].ovlay = (u_int8_t *) malloc(e.ov_siz[i]);
	  if (ovlist[i].ovlay == NULL) {
	    fprintf(stderr, "Apout - can't malloc overlay!\n");
	    exit(EXIT_FAILURE);
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

				/* And deal with the emt instructions */
	for (i = 544; i < 548; i++)
	  itab[i] = do_bsd_overlay;
    }
#endif

  				/* Now load the data into dbase */
  if (dbase)
    for (size = e.a_data; size;) {
      i = (int) fread(dbase, 1, (size_t) size, zin);
      if (i == -1) { (void) fclose(zin); return (i); }
      size -= i;
      dbase += i;
    }

  				/* Now clear the bss */
  if ((bbase != 0) && (e.a_bss != 0))
    memset(bbase, 0, (size_t) e.a_bss);


  /* Set up the registers and flags, and the stack */
  (void) fclose(zin);
  sim_init();
  regs[PC] = e.a_entry;
  if( Binary == IS_A68 ) {
      regs[5]= e.max_ovl;
      regs[4]= 0160000;
  }
  set_arg_env(want_env);
  return (0);
}

/*
 * C runtime startoff.	When an a.out is loaded by the kernel, the kernel
 * sets up the stack as follows:
 *
 *	_________________________________
 *	| (NULL)			| 0177776: top of memory
 *	|-------------------------------|
 *	|				|
 *	| environment strings		|
 *	|				|
 *	|-------------------------------|
 *	|				|
 *	| argument strings		|
 *	|				|
 *	|-------------------------------|
 *	| envv[envc] (NULL)		| end of environment vector tag, a 0
 *	|-------------------------------|
 *	| envv[envc-1]			| pointer to last environment string
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| envv[0]			| pointer to first environment string
 *	|-------------------------------|
 *	| argv[argc] (NULL)		| end of argument vector tag, a 0
 *	|-------------------------------|
 *	| argv[argc-1]			| pointer to last argument string
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| argv[0]			| pointer to first argument string
 *	|-------------------------------|
 * sp-> | argc				| number of arguments
 *	---------------------------------
 *
 * Crt0 simply moves the argc down two places in the stack, calculates the
 * the addresses of argv[0] and envv[0], putting those values into the two
 * spaces opened up to set the stack up as main expects to see it.
 *
 * If want_env is set, create a stack by including environment variables:
 * used by V7, 2.9BSD, 2.11BSD. Otherwise, don't create environment
 * variables: used by V1 up to V6.
 */
static void set_arg_env(int want_env)
{
  int i, posn, len;
  int eposn[MAX_ARGS];
  int aposn[MAX_ARGS];

  				/* Set default environment if there is none */
  if (Envp[0] == NULL) {
    Envc = default_envc;
    for (i = 0; i < Envc; i++)
      Envp[i] = default_envp[i];
  }
#ifdef DEBUG
  			/* Set up the program's name -- used for debugging */
  if (progname) free(progname);
  progname = strdup(Argv[0]);

  if (trap_debug) {
    fprintf(dbg_file, "In set_arg_env, Argc is %d\n", Argc);
    for (i = 0; i < Argc; i++)
      fprintf(dbg_file, "  Argv[%d] is %s\n", i, Argv[i]);
    for (i = 0; i < Envc; i++)
      fprintf(dbg_file, "  Envp[%d] is %s\n", i, Envp[i]);
  }
#endif

  			/* Now build the arguments and pointers on the stack */

#ifdef EMUV1
  if ((Binary == IS_V1) || (Binary == IS_V2))
    posn = KE11LO - 2;				/* Start below the KE11A */
  else
#endif
    posn = PDP_MEM_SIZE - 2;
  sl_word(posn, 0);				/* Put a NULL on top of stack */

  if (want_env == 1)
    for (i = Envc - 1; i != -1; i--) {		/* For each env string */
      len = strlen(Envp[i]) + 1;		/* get its length */
      posn -= len;
      memcpy(&dspace[posn], Envp[i], (size_t) len);
      eposn[i] = posn;
    }

  for (i = Argc - 1; i != -1; i--) {		/* For each arg string */
    len = strlen(Argv[i]) + 1;			/* get its length */
    posn -= len;
    memcpy(&dspace[posn], Argv[i], (size_t) len);
    aposn[i] = posn;
  }
  posn -= 2;
  sl_word(posn, 0);			/* Put a NULL at end of env array */

  if (want_env == 1) {			/* For each env string */
    for (i = Envc - 1; i != -1; i--) {
      posn -= 2; 			/* put a pointer to the string */
      sl_word(posn, (u_int16_t) eposn[i]);
    }
    posn -= 2;
  }
  					/* Put a NULL or -1 before arg ptrs */
  if (want_env == 0) sl_word(posn, -1)
  else sl_word(posn, 0);

  for (i = Argc - 1; i != -1; i--) {		/* For each arg string */
    posn -= 2;
    sl_word(posn, (u_int16_t) aposn[i]);	/* put a ptr to the string */
  }
  posn -= 2;
  sl_word(posn, (u_int16_t) Argc);		/* Save the count of args */
  regs[SP] = (u_int16_t) posn;			/* and initialise the SP */
}


#ifdef EMU211
/* This function probably belongs in bsdtrap.c, but all the vars are
 * here, so why not!
 *
 * Deal with overlay changes which come in via an emt instruction.
 */

void do_bsd_overlay()
{
  int ov = regs[0] - 1;

  if (ovlist[ov].size == 0) {
    fprintf(stderr, "Apout - can't switch to empty overlay %d\n", ov);
    exit(EXIT_FAILURE);
  }
  JsrDebug((dbg_file, "switching to overlay %d\n", ov));

  /* Memcpy overlay into main ispace */
  memcpy(ovbase, ovlist[ov].ovlay, ovlist[ov].size);
  ov_changes++;
  current_ov = ov;
}
#endif
