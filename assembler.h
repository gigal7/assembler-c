#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include "defs.h"
#include "symtable.h"

#define BIT(n)                   (1 << (n))

#define LEGAL_ADDRMODE_0123     (BIT(0)|BIT(1)|BIT(2)|BIT(3))
#define LEGAL_ADDRMODE_123      (BIT(1)|BIT(2)|BIT(3))
#define LEGAL_ADDRMODE_12       (BIT(1)|BIT(2))
#define LEGAL_ADDRMODE_NONE     0

struct assembler_state {
	int IC;
	int DC;
	int line_number;
	symtab_t symbols;
	const char *filename;
	short code[LENGTH_MEMORY];
	short data[LENGTH_MEMORY];
};

struct operation_info {
	const char   *name;
	int          opcode;
	symbol_type_t symtype;
	int          legal_addrmode_1st_op;
	int          legal_addrmode_2nd_op;
	int          (*parse)(operation_info_t*, assembler_state_t*, char*);
};

struct operand_info{
	operand_type_t type;
	union {
		int  immediate;
		const char *label;
		struct
		{
			char *label;
			int field_number;
		} struc;
		int  register_id;
	}data;
};


operation_info_t *find_operation(char operation[]);
int tokenize_line(char *line, char **label, char **operation, char **operands, assembler_state_t *state);
int check_label(const char label[], assembler_state_t *state);
int get_next_comma(operation_info_t *info, assembler_state_t *state, char **tok1, char **tok2 ,char **operands);
int parse_data(operation_info_t *info, assembler_state_t *state, char *operands);
int parse_entry(operation_info_t *info, assembler_state_t *state, char *operands);

#endif
