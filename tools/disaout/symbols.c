/* Tables and functions to keep track of symbols */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
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

void add_symbol(int addr, int type, int size)
{
  struct symbol *s;

  /* See if we have a symbol already defined at this address */
  if (symtypelist[type].table[addr] != NULL) return;

  /* No, so create one */
  s= malloc(sizeof(struct symbol));
#if 0
  s->name= malloc(12);
  snprintf(s->name,12,symtypelist[type].format, symtypelist[type].counter++);
#endif
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
    s= isym[i]; s->name= malloc(12);
    type=s->type;
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
