/*name: Gal Girad ID 307901199*/
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "defs.h"

#define SYMBOL_HASH_SIZE  101

typedef enum symbol_type {
	SYMBOL_TYPE_UNKNOWN,
	SYMBOL_TYPE_CODE,
	SYMBOL_TYPE_DATA,
	SYMBOL_TYPE_EXTERNAL
} symbol_type_t;


typedef struct relocation relocation_t;
struct relocation {
    int          ic; /* Which place in code should be update */
    relocation_t *next;
};


typedef struct symbol symbol_t;
struct symbol {
	char          name[MAX_LABEL_LENGTH];
	symbol_type_t type;
    int           index; /* ic or dc */
    relocation_t  *relocations;
    int           is_entry;
    symbol_t      *next; /* Next in hash */
};


typedef symbol_t *symtab_t[SYMBOL_HASH_SIZE];


void symtab_init(symtab_t *t);

void symtab_free(symtab_t *t);

int symtab_new_label(symtab_t *t, const char *name, symbol_type_t type,
			         int ic, int dc);

int symtab_new_operand(symtab_t *t, const char *name, int ic);

int symtab_update_relocations_and_write(symtab_t *t, assembler_state_t *state);
int symtab_new_entry(symtab_t *t, char *name);

#endif
