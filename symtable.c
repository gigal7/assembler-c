/*name: Gal Girad ID 307901199*/
#include "symtable.h"
#include "assembler.h"
#include "defs.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*This method initializes all the symbols table contents to NULL*/
void symtab_init(symtab_t *t)
{
	int i;

	for (i = 0; i < SYMBOL_HASH_SIZE; i++) {
		(*t)[i] = NULL;
	}
}
/*This method frees the symbols table contents */
void symtab_free(symtab_t *t)
{
	relocation_t *r;
	symbol_t *s;
	int bucket;

	for (bucket = 0; bucket < SYMBOL_HASH_SIZE; bucket++) {
		while ((*t)[bucket] != NULL) {
			s = (*t)[bucket];
			(*t)[bucket] = s->next;

			while (s->relocations !=NULL) {
				r = s->relocations;
				s->relocations = r->next;
				free(r);
			}

			free(s);
		}
	}
}

/*This method form hash value for string name
 * taken from page 144*/
int calc_hash(const char *name)
{
	unsigned hashval;
	const char *p;

	hashval = 0;
	for (p = name; *p != '\0'; p++) {
		hashval = *p + 31 * hashval;
	}
	return hashval % SYMBOL_HASH_SIZE;
}

/*This method finds a symbol name in the list
 * returns a structure of symbol_t in case of success and NULL otherwise*/
symbol_t *find_in_bucket(symtab_t *t, int bucket, const char *name)
{
	symbol_t *s;

	for (s = (*t)[bucket]; s != NULL; s = s->next) {
		if (!strcmp(name, s->name)) {
			return s;
		}
	}
	return NULL;
}
/*This method adds new symbol to the list
 * returns 0 in case of success and -1 otherwise*/
int add_new_symbol(symtab_t *t, int bucket, const char *name, symbol_t **newsym)
{
	symbol_t *s;

	s = malloc(sizeof (*s));
	if (s == NULL) {
		fprintf(stderr, "Failed to allocate symbol\n");
		return -1;
	}

	strcpy(s->name, name);
	s->relocations = NULL;
	s->type        = SYMBOL_TYPE_UNKNOWN;

	/* add to list */
	s->next = (*t)[bucket];
	(*t)[bucket] = s;
	*newsym = s;

	return 0;
}

/*This method checks whether a label name was declared
 * if declared before ,returns -1.
 * if not - adds the name, the address of the new symbol to the list and returns 0 */
int symtab_new_label(symtab_t *t, const char *name, symbol_type_t type,
		             int ic, int dc)
{
	int bucket;
	symbol_t *s;
	int ret;

	bucket = calc_hash(name);
	s = find_in_bucket(t, bucket, name);
	if (s != NULL) {
		if (s->type != SYMBOL_TYPE_UNKNOWN) {
			fprintf(stderr, "Label %s re-defined\n", name);
			return -1;
		}
	} else {
		ret = add_new_symbol(t, bucket, name, &s);
		if (ret < 0) {
			return ret;
		}
	}

	s->type  = type;
	switch (type) {
	case SYMBOL_TYPE_CODE:
		s->index = ic;
		break;
	case SYMBOL_TYPE_DATA:
		s->index = dc;
		break;
	default:
		s->index = 0;
		break;
	}

	return 0;
}
/*This method checks whether a label name(operand of .entry) was declared.
 * if was not declared and fails to add to symbol table returns -1
 * otherwise adds the name, the address of the new symbol to the list, turn the flag "is entry" to 1 and returns 0
 * */
int symtab_new_entry(symtab_t *t, char *name) {
	int bucket;
	symbol_t *s;
	int ret;

	bucket = calc_hash(name);

	s = find_in_bucket(t, bucket, name);
	if(s == NULL) {
		ret = add_new_symbol(t, bucket, name, &s);
		if(ret < 0)
			return ret;
	}
	s->is_entry = 1;
	return 0;
}

/*This method handles a symbol given as operand - if symbol name was not found in the symbols list,
 *add it with type unknown, and allocate new memory to remember the address of this symbol.
 *set the next of r to point on  s->relocations and update s->relocations to point on r.
 *return 0 in case of success and -1 otherwise*/
int symtab_new_operand(symtab_t *t, const char *name, int ic)
{
	int bucket;
	relocation_t *r;
	symbol_t *s;
	int ret;

	bucket = calc_hash(name);

	s = find_in_bucket(t, bucket, name);
	if (s == NULL) {
		ret = add_new_symbol(t, bucket, name, &s);
		if (ret < 0) {
			return ret;
		}
	}

	r = malloc(sizeof(*r));
	if (r == NULL) {
		fprintf(stderr, "Failed to allocate relocation\n");
		return -1;
	}

	r->ic = ic;

	/* add r to s */
	r->next = s->relocations;
	s->relocations = r;

	return 0;
}
/*This method updates all the relocation addresses to the actual address of the label
 * return 0 in case of success and -1 otherwise*/
int symtab_update_relocations_and_write(symtab_t *t, assembler_state_t *state)
{
	FILE *entfile = NULL, *extfile = NULL;
	char base32[3];
	relocation_t *r;
	symbol_t *s;
	int bucket;
	int word;
	int address;

	for (bucket = 0; bucket < SYMBOL_HASH_SIZE; bucket++) {
		while ((*t)[bucket] != NULL) {
			s = (*t)[bucket];
			(*t)[bucket] = s->next;

			switch (s->type) {
			case SYMBOL_TYPE_UNKNOWN:
				fprintf(stderr, "Unresolved symbol %s\n", s->name);
				return -1;
			case SYMBOL_TYPE_CODE:
				address = ASSEMBLY_CODE_START_ADDRESS + s->index;
				word = (address << 2) | ARE_RELOC;
				break;
			case SYMBOL_TYPE_DATA:
				address = ASSEMBLY_CODE_START_ADDRESS + state->IC + s->index;
				word = (address << 2) | ARE_RELOC;
				break;
			case SYMBOL_TYPE_EXTERNAL:
				if (s->is_entry) {
					fprintf(stderr, "Symbol %s cannot be both external and entry\n", s->name);
					return -1;
				}
				address = 0;
				word = (address << 2) | ARE_EXTERN; /*  External */
				break;
			}

			while (s->relocations != NULL) {
				r = s->relocations;
				s->relocations = r->next;
				state->code[r->ic] = word; /* Update operand */

				if (s->type == SYMBOL_TYPE_EXTERNAL) {
					if (extfile == NULL) {
						extfile = open_file_with_ext(state->filename, "ext", "w");
						if (extfile == NULL) {
							return -1;
						}
					}

					to_base32(r->ic + ASSEMBLY_CODE_START_ADDRESS, base32);
					fprintf(extfile, "%s %s\n", s->name, base32);
				}

				free(r);
			}

			if (s->is_entry) {
				if (entfile == NULL) {
					entfile = open_file_with_ext(state->filename, "ent", "w");
					if (entfile == NULL) {
						return -1;
					}
				}

				to_base32(address, base32);
				fprintf(entfile, "%s %s\n", s->name, base32);
			}

			free(s);
		}
	}

	if (extfile != NULL) {
		fclose(extfile);
	}
	if (entfile != NULL) {
		fclose(entfile);
	}
	return 0;
}


