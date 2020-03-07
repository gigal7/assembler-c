/*name: Gal Girad ID 307901199*/
#ifndef DND_H
#define DND_H


#include <stdio.h>

/*constants*/
#define LENGTH_MEMORY 	256
#define MAX_PATH   128 /*The maximum length of the file name*/
#define MAX_LINE_LENGTH 80 /*The maximum length of lines in file*/
#define ASSEMBLY_CODE_START_ADDRESS 100 /*The starting address is 100 in decimal */
#define END_OF_TOKENS -2  /*A sign that says that there are not tokens left*/
#define MAX_NUMBER_OF_SYMBOL 256 /*The maximum number of symbols that can be */
#define MAX_LABEL_LENGTH  30
#define LENGTH_OF_OPS 21 /*There are 21 operations*/
/*ARE bits*/
#define ARE_FIXED  0
#define ARE_EXTERN 1
#define ARE_RELOC  2

typedef enum operand_type{
	ADDR_IMMEDIATE = 0,
	ADDR_DIRECT    = 1,
	ADDR_STRUCT    = 2,
	ADDR_REGISTER  = 3
} operand_type_t;

typedef struct assembler_state assembler_state_t;
typedef struct operation_info operation_info_t;
typedef struct operand_info operand_info_t;

FILE *open_file_with_ext(const char *filename, const char *ext, const char *mode);
void to_base32(int x, char *str);
int my_atoi(assembler_state_t *state, char *number_str, int *number);

#endif

