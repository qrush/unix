/* Tables and functions to keep track of symbols */

#include "aout.h"

struct symbol * isym[PDP_MEM_SIZE],  * dsym[PDP_MEM_SIZE];

/* Array of structs holding details for each type of symbol */
struct symtype {
  char *format;
  int counter;
  struct symbol **table;
} symtypelist[] = {
	{ "%d", 1, isym },
	{ "func%d", 1, isym },
	{ "data%d", 1, dsym },
	{ "jsrtext%d", 1, isym },
	{ "jsrdata%d", 1, isym },
};

/* Debug code */
void print_symtables(void)
{
  int i;
  for (i=0; i< PDP_MEM_SIZE; i++) {
    if (isym[i]!=NULL)
      printf("0%06o %d %d %s\n",i, isym[i]->type, isym[i]->size, isym[i]->name);
    if (dsym[i]!=NULL)
      printf("0%06o %d %d %s\n",i, dsym[i]->type, dsym[i]->size, dsym[i]->name);
  }
}

void add_symbol(int addr, int type, int size)
{
  struct symbol *s;

  /* See if we have a symbol already defined at this address */
  /* If there is one and its type is >= ours, don't put a new one in */
  if ((symtypelist[type].table[addr] != NULL) &&
      (symtypelist[type].table[addr]->type >= type)) return;

  /* No, so create one */
  s= malloc(sizeof(struct symbol));
#if 0
  s->name= malloc(12);
  snprintf(s->name,12,symtypelist[type].format, symtypelist[type].counter++);
#endif
  s->name= NULL;			/* To be filled in later */
  s->type= type;
  s->size= size;
  symtypelist[type].table[addr]= s;
}

/* Walk through both isym and dsym tables, giving them actual
 * symbol names.
 */
void patch_symbols(void)
{
  int i,type;
  struct symbol *s;
  for (i=0; i< PDP_MEM_SIZE; i++) {
    if (isym[i]==NULL) continue;
    s= isym[i];
    if (s->name != NULL) continue;
    type=s->type;
    s->name= malloc(12);
    snprintf(s->name,12,symtypelist[type].format, symtypelist[type].counter++);
  }
  for (i=0; i< PDP_MEM_SIZE; i++) {
    if (dsym[i]==NULL) continue;
    s= dsym[i]; s->name= malloc(12);
    type=s->type;
    snprintf(s->name,12,symtypelist[type].format, symtypelist[type].counter++);
  }
}

struct symbol * get_isym(int addr)
{
  return(isym[addr]);
}

struct symbol * get_dsym(int addr)
{
  return(dsym[addr]);
}

/* Walk the 0407 symbol table and load the symbols found */
void load_0407_symbols(FILE *zin, int offset, int size, int base)
{
  struct sym0407 S;
  struct symbol *oursym;
  char name[9];
  long curpos;

  /* Record where we are, and seek to the symbol table */
  curpos= ftell(zin);
  if (fseek(zin, offset, SEEK_SET) != 0) {
    printf("Unable to load symbol table at offset %d\n", offset);
    return;
  }
  name[8]='\0';		/* Ensure we get a properly terminated string */

  /* Walk the list */
  while (size>0) {
    fread(&S, sizeof(S), 1, zin); size -= sizeof(S);
    memcpy(name, S.name, 8);
    S.addr += base;

    switch (S.type) {
      case ASYM_DATA:
      case ASYM_BSS:
      case ASYM_DATAEXT:
      case ASYM_BSSDEXT:
			oursym= malloc(sizeof(struct symbol));
  			oursym->name= strdup(name);
  			oursym->type= SYM_DATA;
  			oursym->size= 0;
  			dsym[S.addr]= oursym;
			break;

      case ASYM_TEXT:
      case ASYM_TEXTEXT:
			oursym= malloc(sizeof(struct symbol));
  			oursym->name= strdup(name);
			/* If it starts with l[0-9], it's a branch */
			if (name[0]=='l' && name[1] >= '0' &&  name[1] <= '9')
  			  oursym->type= SYM_BRANCH;
			else
  			  oursym->type= SYM_FUNCTION;
  			oursym->size= 0;
  			isym[S.addr]= oursym;
			break;
    }
  }
  fseek(zin, curpos, SEEK_SET);
}
