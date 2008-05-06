#include "aout.h"

extern int load_a_out(const char *file, struct exec *E);
extern int printins(int addr);
extern void patch_symbols(void);
extern void print_symtables(void);
extern u_int8_t *ispace, *dspace;	/* Instruction and Data spaces */
extern int doprint;
int onepass = 0;		/* Only do a single pass */
int printaddrs = 0;		/* Print out addresses and words */

void dopass(struct exec *e)
{
  int i;
  u_int16_t *iptr;
  for (i = e->a_entry; i < e->a_entry + e->a_text;) {
    iptr = (u_int16_t *) & ispace[i];
    if (doprint && printaddrs)
      printf("%06o: %06o\t", i, *iptr);
    i += printins(i);
  }
}

void usage()
{
  fprintf(stderr, "Usage: disaout [-1a] file\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  struct exec E;
  int ch, err;

  /* Get any arguments */
  while ((ch = getopt(argc, argv, "1a")) != -1) {
    switch (ch) {
    case '1':
      onepass = 1;
      break;
    case 'a':
      printaddrs = 1;
      break;
    case '?':
    default:
      usage();
    }
  }
  argc -= optind;
  argv += optind;


  /* Check we have an file to open */
  if (argc != 1)
    usage();

  /* Get the header details for the a.out file */
  err = load_a_out(argv[0], &E);

  if (err == -1) {
    fprintf(stderr, "%s does not appear to be a PDP-11 a.out file\n", argv[0]);
    exit(1);
  }

  printf("/ text at 0%o, len 0%o, end 0%o\n",
	E.a_entry, E.a_text, E.a_entry + E.a_text);

  if (onepass == 0) {
    doprint = 0;
    dopass(&E);			/* Do pass 1 to infer symbols */
    patch_symbols();
    /* print_symtables(); */
  }
  doprint = 1;
  dopass(&E);			/* Do pass 2 to print it out */

  exit(0);
}
