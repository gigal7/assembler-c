
#include "assembler.h"
#include "symtable.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*This method gets a token not including comma(if that were the case) returns 0 for valid token and -1 otherwise*/
int get_next_token(assembler_state_t *state, char **tok, char **operands)
{
	char *p, *end;

	p = *operands;

	/* Skip leading spaces */
	while (isspace(*p)) {
		p++;
	}
	if (*p == ',') {
		fprintf(stderr, "Invalid comma, line %d\n", state->line_number);
		return -1;
	}

	if (*p == '\0') { /*There are no operands, so initialize tok to be NULL and return a number that signs that the tok is found*/
		*tok = NULL;
		return END_OF_TOKENS;
	}

	/* The token begins now */
	*tok = p;

	if (*p == '"') { /* In case of a string */
		do {
			p++;
			if (*p == '\0') {
				fprintf(stderr, "Missing \", line %d\n", state->line_number);
				return -1;
			}
		} while (*p != '"');
		p++; /* Skip last '"' */
	} else { /* Non-string token */
		do {
			p++;
		} while (!isspace(*p) && (*p != '\0') && (*p != ','));
	}

	/* Token ends here */
	end = p;

	/* Skip trailing spaces */
	while (isspace(*p)) {
		p++;
	}
	if (*p == ',') {
		p++;
		while (isspace(*p)) {
			p++;
		}
		if (*p == '\0') {
			fprintf(stderr, "Invalid comma in line end, line %d\n", state->line_number);
			return -1;
		}
	} else if (*p != '\0') {
		/* Not end and not a comma - error */
		fprintf(stderr, "Unexpected token, line %d\n", state->line_number);
		return -1;
	}

	*end = '\0'; /* Close the token */

	*operands = p;
	return 0;
}

/*This method gets the next number and checks if a number is valid returns 0 for valid and -1 otherwise*/
int get_next_number(assembler_state_t *state, int *number, char **operands) {
	char *number_str;
	int ret;

	ret = get_next_token(state, &number_str, operands);
	if (ret < 0) { /*The method "get_next_token" already gives error prints*/
		return ret;
	}

	return my_atoi(state, number_str, number);
}

/*This method adds a word to code array and increment the ic value*/
void emit_code(assembler_state_t *state, int word) {
	word = word & 1023;
	state->code[state->IC++] = word;
}

/*This method adds a symbol to code array and increment the ic value
 returns 0 in case of emit success and -1 otherwise*/
int emit_relocation(assembler_state_t *state, const char *symbol_name) {
	int ret;

	ret = symtab_new_operand(&state->symbols, symbol_name, state->IC);
	if (ret < 0) {
		return ret;
	}

	emit_code(state, 0);
	return 0;
}

/*This method adds a number to data array and increment the dc value*/
void emit_data(assembler_state_t *state, int number)
{
	number = number & 1023; /*Use of '&' to mask off all the other bits except the 10 first ones*/
	state->data[state->DC++] = number;
}

/*This method adds a string to data array and increment the dc value*/
void emit_data_string(assembler_state_t *state, const char *string)
{
	do {
		emit_data(state, *(string++));
	} while (*string != '\0');
	emit_data(state, 0);
}

/*This method parse the data operation and checks for mistakes
 * returns 0 in case of parse success and -1 otherwise*/
int parse_data(operation_info_t *info, assembler_state_t *state, char *operands)
{
	int number, ret;

	/*There must be at least one operand in .data operation*/
	ret = get_next_number(state, &number, &operands);
	if (ret < 0) { /*The method "get_next_token" already gives error prints*/
		return ret;
	}
	for (;;) {
		emit_data(state, number);

		ret = get_next_number(state, &number, &operands);
		if (ret == END_OF_TOKENS) {
			return 0; /* End, success */
		} else if (ret < 0) {
			return ret; /* Error */
		}
		/* Otherwise - get another number and continues */
	}
	return 0;
}

/*This method gets the next string and checks if a string is valid
 * it is called last_string because a string must be the last operand in a line
 * returns 0 in case of success and -1 otherwise */
int get_next_and_last_string(assembler_state_t *state, char **string, char **operands)
{
	char *s;
	int ret;
	int len;

	ret = get_next_token(state, &s, operands);
	if (ret < 0) { /*The method "get_next_token" already gives error prints*/
		return ret;
	}
	if (*s == '"') {
		++s;
	} else {
		fprintf(stderr, "String must begin with apostrophes, line %d\n", state->line_number);
		return -1;
	}

	len = strlen(s);
	if (len > 0 && s[len - 1] == '"') {
		s[len - 1] = '\0';
	} else {
		fprintf(stderr, "String must end with apostrophes, line %d\n", state->line_number);
		return -1;
	}

	*string = s;

	/* Make sure no more tokens */
	ret = get_next_token(state, &s, operands);
	if (ret == 0) {
		fprintf(stderr, "Too many tokens for string, line %d\n", state->line_number);
		return -1;
	}

	return 0;
}

/*This method parse a string, adding the string to the data array and checks for mistakes
 * returns 0 in case of parse success and -1 otherwise*/
int parse_string(operation_info_t *info, assembler_state_t *state, char *operands)
{
	char *string;
	int ret;

	ret = get_next_and_last_string(state, &string, &operands);
	if (ret < 0) { /*The method "get_next_and_last_string" already gives error prints*/
		return ret;
	}
	emit_data_string(state, string);

	return 0;
}

/*This method parse a .struct operation, checks for mistakes afterwards adds the struct operands to data array
 * returns 0 in case of parse success and -1 otherwise*/
int parse_struct(operation_info_t *info, assembler_state_t *state, char *operands)
{
	int number, ret;
	char *string;

	/* The first operand is a number*/
	ret = get_next_number(state, &number, &operands);
	if (ret < 0) /*The method "get_next_token" already gives error prints*/
		return ret;

	emit_data(state, number);

	ret = get_next_and_last_string(state, &string, &operands);
	if (ret < 0) /*The method "get_next_and_last_string" already gives error prints*/
		return ret;
	/* The second operand is a string*/
	emit_data_string(state, string);

	return 0; /*Success*/
}

/*This method parse an operand and checks which addressing method it belongs to returns 0 in case of parse success
 * and -1 otherwise */
int parse_operand(assembler_state_t *state, char *operand_str, operand_info_t *opinfo)
{
	int register_id, ret;
	char *p;

	if ((operand_str[0] == 'r') && (strlen(operand_str) == 2)) {
		register_id = (operand_str[1] - '0'); /*Makes a number as string to a number as integer*/
		if (register_id >= 0 || register_id < 8) {
			opinfo->type = ADDR_REGISTER;
			opinfo->data.register_id = register_id;
			return 0;
		} else {
			fprintf(stderr, "Invalid register name, line %d\n", state->line_number);
			return -1;
		}
	}

	if (operand_str[0] == '#') {
		opinfo->type = ADDR_IMMEDIATE;
		return my_atoi(state, operand_str + 1, &opinfo->data.immediate);
	}

	p = strchr(operand_str, '.');
	if (p != NULL) {
		opinfo->type = ADDR_STRUCT;
		*p = '\0'; /*End the name of the struct*/
		ret = check_label(operand_str, state);
		if (ret < 0) {
			return ret;
		}
		opinfo->data.struc.label = operand_str;

		ret = my_atoi(state, p + 1, &opinfo->data.struc.field_number);
		if (ret < 0) {
			return ret;
		}

		if (opinfo->data.struc.field_number !=1 && opinfo->data.struc.field_number != 2) {
			fprintf(stderr, "Illegal filed number, line %d\n", state->line_number);
			return -1;
		}

		return 0;
	}

	if (check_label(operand_str, state) == 0) {
		opinfo->type = ADDR_DIRECT;
		opinfo->data.label = operand_str;
		return 0;
	}

	/*The operand does not fit to any addressing methods*/
	fprintf(stderr, "Invalid operand, line %d\n", state->line_number);
	return -1;
}

/*This method emits the opcode to the code array
 * returns 0 in case of emit success and -1 otherwise*/
void emit_opcode(operation_info_t *info, assembler_state_t *state, operand_info_t opinfo[], int n)
{
	int word;
	/* Build first word of the operation */
	if (n == 1) {
		word = opinfo[0].type << 2; /* Fill bits 2,3 - single destination operand */
	} else if (n == 2) {
		word = (opinfo[0].type << 4) | /* Fill bits 4,5 - source operand is first */
			   (opinfo[1].type << 2); /* Fill bits 2,3 - dest operand is second */
	} else {
		word = 0;
	}
	word |= info->opcode << 6; /* Add opcode */

	/* Emit opcode word */
	emit_code(state, word);
}

/*This method emits a given number of operands to code array
 * returns 0 in case of emit success and -1 otherwise*/
int emit_n_operands(operation_info_t *info, assembler_state_t *state, operand_info_t opinfo[], int n)
{
	int word;
	int i;
	int ret;

	if (n >= 1) {
		if ((BIT(opinfo[0].type) & info->legal_addrmode_1st_op) == 0) {
			fprintf(stderr, "Illegal addressing mode of 1st operand, line %d\n", state->line_number);
			return -1;
		}
	}
	if (n >= 2) {
		if ((BIT(opinfo[1].type) & info->legal_addrmode_2nd_op) == 0) {
			fprintf(stderr, "Illegal addressing mode of 2nd operand, line %d\n", state->line_number);
			return -1;
		}
	}

	/* Special case - 2 registers */
	if (n == 2 && opinfo[0].type == ADDR_REGISTER && opinfo[1].type == ADDR_REGISTER) {
		word = (opinfo[1].data.register_id << 2) | /* 2-5 - Dest operand */
			   (opinfo[0].data.register_id << 6);  /* 6-9 - Source operand */
		emit_code(state, word);
		return 0;
	}

	for (i = 0; i < n; i++) {
		switch (opinfo[i].type) {
		case ADDR_IMMEDIATE:
			word = opinfo[i].data.immediate << 2; /*ARE=00 */
			emit_code(state, word);
			break;
		case ADDR_DIRECT:
			ret = emit_relocation(state, opinfo[i].data.label);
			if (ret < 0) {
				return ret;
			}
			break;
		case ADDR_STRUCT:
			ret = emit_relocation(state, opinfo[i].data.struc.label);
			if (ret < 0) {
				return ret;
			}
			word = opinfo[i].data.struc.field_number << 2; /* Emit field number */
			emit_code(state, word); /*ARE=00 */
			break;
		case ADDR_REGISTER:
			if (i == (n-1))
				word = opinfo[i].data.register_id << 2; /* Dest operand */
			else
				word = opinfo[i].data.register_id << 6; /* Source operand */

			emit_code(state, word); /*ARE=00 */
			break;
		}
	}

	return 0;
}

/*This method parse a given number of operands and then first emits the
 *  opcode to code array and second emits the operands to code array
 *  returns 0 in case of parse success and -1 otherwise*/
int parse_n_operands(operation_info_t *info, assembler_state_t *state, char *operands, int n) {
	operand_info_t opinfo[2]; /*There are two fields(operands) in operation_info struct - one is a number second a string*/
	char *operand_str;
	int i, ret;

	for (i = 0; i < n; i++) {
		ret = get_next_token(state, &operand_str, &operands);
		if (ret < 0) {/*The method "get_next_token" already gives error prints*/
			return ret;
		}

		ret = parse_operand(state, operand_str, &opinfo[i]);
		if (ret < 0) {
			return ret;
		}
	}

	ret = get_next_token(state, &operand_str, &operands);
	if (ret == 0) {
		fprintf(stderr, "Too many operands, line %d\n", state->line_number);
		return -1;
	}

	emit_opcode(info, state, opinfo, n);
	return emit_n_operands(info, state, opinfo, n);
}

/*This method parse 0 operands returns 0 in success and -1 otherwise*/
int parse_0operands(operation_info_t *info, assembler_state_t *state, char *operands) {
	return parse_n_operands(info, state, operands, 0);
}

/*This method parse 1 operands returns 0 in success and -1 otherwise*/
int parse_1operands(operation_info_t *info, assembler_state_t *state, char *operands) {
	return parse_n_operands(info, state, operands, 1);
}

/*This method parse 2 operands returns 0 in success and -1 otherwise
 * returns 0 in case of parse success and -1 otherwise*/
int parse_2operands(operation_info_t *info, assembler_state_t *state,  char *operands) {
	return parse_n_operands(info, state, operands, 2);
}

/*This method parse an .entry operation, checks for mistakes,
 *  if the operand is valid adds it to data array
 *  returns 0 in case of parse success and -1 otherwise*/
int parse_entry(operation_info_t *info, assembler_state_t *state, char *operands) {
	int ret;
	char *operand_str;

	ret = get_next_token(state, &operand_str, &operands);
	if (ret < 0) { /*The method "get_next_token" already gives error prints*/
		return ret;
	}
	ret = check_label(operand_str, state);
	if (ret < 0) { /*The method "check_label" already gives error prints*/
		return ret;
	}

	ret =  symtab_new_entry(&(state->symbols), operand_str);
	if (ret < 0) {
		return ret;
	}

	ret = get_next_token(state, &operand_str, &operands);
	if (ret == 0) {
		fprintf(stderr, "Too many operands, line %d\n", state->line_number);
		return -1;
	}

	return 0;
}

/*This method parse an .extern operation, checks for mistakes,
 *  if the operand is valid adds it to data array
 *  returns 0 in case of parse success and -1 otherwise*/
int parse_extern(operation_info_t *info, assembler_state_t *state, char *operands) {
	char *operand_str;
	int ret;

	ret = get_next_token(state, &operand_str, &operands);
	if (ret < 0) {/*The method "get_next_token" already gives error prints*/
		return ret;
	}
	ret = check_label(operand_str, state);
	if (ret < 0) {/*The method "check_label" already gives error prints*/
		return ret;
	}
	ret = symtab_new_label(&state->symbols, operand_str,
			               SYMBOL_TYPE_EXTERNAL, 0, 0);
	if (ret < 0) { /*The method symtab_new_label already gives specified error*/
		return ret;
	}

	ret = get_next_token(state, &operand_str, &operands);
	if (ret == 0) {
		fprintf(stderr, "Too many operands, line %d\n", state->line_number);
		return -1;
	}

	return 0; /*Parse operand succeed*/
}

/*A structure of operands and their information
 * 1)the name of the operation
 * 2)the op code
 * 3)if there is a symbol - what kind it should be
 * 4)what is the legal address mode for source address
 * 5)what is the legal address mode for destination address
 * 6) what should be the kind of operands the operation gets*/
operation_info_t ops[] = {
	{"mov",     0, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_0123, LEGAL_ADDRMODE_123,  parse_2operands},
	{"cmp",     1, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_0123, LEGAL_ADDRMODE_0123, parse_2operands},
	{"add",     2, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_0123, LEGAL_ADDRMODE_123,  parse_2operands},
	{"sub",     3, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_0123, LEGAL_ADDRMODE_123,  parse_2operands},
	{"lea",     6, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_12, LEGAL_ADDRMODE_123,  parse_2operands},
	{"not",     4, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_123, LEGAL_ADDRMODE_NONE, parse_1operands},
	{"clr",     5, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_123, LEGAL_ADDRMODE_NONE, parse_1operands},
	{"inc",     7, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_123, LEGAL_ADDRMODE_NONE, parse_1operands},
	{"dec",     8, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_123, LEGAL_ADDRMODE_NONE, parse_1operands},
	{"jmp",     9, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_123, LEGAL_ADDRMODE_NONE, parse_1operands},
	{"bne",    10, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_123, LEGAL_ADDRMODE_NONE, parse_1operands},
	{"red",    11, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_123, LEGAL_ADDRMODE_NONE, parse_1operands},
	{"prn",    12, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_0123, LEGAL_ADDRMODE_NONE, parse_1operands},
	{"jsr",    13, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_123, LEGAL_ADDRMODE_NONE, parse_1operands},
	{"rts",    14, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_NONE, LEGAL_ADDRMODE_NONE, parse_0operands},
	{"stop",   15, SYMBOL_TYPE_CODE, LEGAL_ADDRMODE_NONE, LEGAL_ADDRMODE_NONE, parse_0operands},
	{".data",   0, SYMBOL_TYPE_DATA, LEGAL_ADDRMODE_NONE, LEGAL_ADDRMODE_NONE, parse_data},
	{".string", 0, SYMBOL_TYPE_DATA, LEGAL_ADDRMODE_NONE, LEGAL_ADDRMODE_NONE, parse_string},
	{".struct", 0, SYMBOL_TYPE_DATA, LEGAL_ADDRMODE_NONE, LEGAL_ADDRMODE_NONE, parse_struct},
	{".entry",  0, SYMBOL_TYPE_UNKNOWN, LEGAL_ADDRMODE_NONE, LEGAL_ADDRMODE_NONE, parse_entry},
	{".extern", 0, SYMBOL_TYPE_UNKNOWN, LEGAL_ADDRMODE_NONE, LEGAL_ADDRMODE_NONE, parse_extern},
	{NULL, -1, -1, LEGAL_ADDRMODE_NONE, LEGAL_ADDRMODE_NONE, NULL}
};

/*Finds if the given operation string exist in operations structure
 * returns a pointer to the appropriate structure or NULL if not found */
operation_info_t *find_operation(char operation[])
{
	int i;

	for (i = 0; ops[i].name != NULL; i++) {
		if (!strcmp(ops[i].name, operation)) {
			return &ops[i];
		}
	}
	return NULL;
}

/*Checks if a label given is as defined in the instructions,
 *  returns 0 for good label and -1 for a bad label and prints errors if there are any*/
int check_label(const char label[], assembler_state_t *state)
{
	const char *p;
	int i;

	p = label; /*A pointer that points on the first character of a label*/

	if(strlen(p) <= 0){
		fprintf(stderr, "No label found, line %d\n", state->line_number);
		return -1;
	} if(isalpha(*p) == 0) {
		fprintf(stderr, "The label does not start with an alphabet, line %d\n", state->line_number);
		return -1;
	} if(strlen(p) >= MAX_LABEL_LENGTH) {
		fprintf(stderr, "The label is too long, line %d\n", state->line_number);
		return -1;
	}

	/*Checks if not operation name or directive name*/
	for(i = 0; i < LENGTH_OF_OPS; i++) {
		if(strcmp(label, ops[i].name) == 0) {
			fprintf(stderr, "The name of the label matches operation name or directing operation name, line %d\n",
					state->line_number);
			return -1;
		}
	}

	/*Checks if not a register name*/
	if(strcmp(label, "r0") == 0 || strcmp(label, "r1") == 0 ||strcmp(label, "r2") == 0 || strcmp(label, "r3") == 0 ||
	   strcmp(label, "r4") == 0 || strcmp(label, "r5") == 0 ||strcmp(label, "r6") == 0 || strcmp(label, "r7") == 0 )
	{
		fprintf(stderr, "The name of a label matches a register name, line %d\n", state->line_number);
		return -1;
	}

	/*Check if all the characters are made of digits and alphabet*/
	while(*p != '\0'){
		if(!isalpha(*p) && !isdigit(*p)){
			fprintf(stderr, "The label doesn't consist only of digits and alphabet, line %d\n", state->line_number);
			return -1;
		}
		p++;
	}
	return 0; /*The label is good */
}

/*This method divides the line into label, operation and operands
 * returns  0 if dividing succeeded and -1 if not */
int tokenize_line(char *line, char **label, char **operation, char **operands, assembler_state_t *state)
{
	char *p, *start;
	int ret;

	*label     = NULL;
	*operation = NULL;
	*operands  = NULL;

	/* Skip leading spaces */
	p = line;
	while (isspace(*p)) {
		p++;
	}
	/* Empty line */
	if (*p == '\0') {
		return 0;
	}
	/* Read first word which can be either label or operation */
	start = p;
	while (isalnum(*p) || (*p == '.')) {
		++p;
	}
	/*In case of label*/
	if (*p == ':')
	{
		*(p++) = '\0';
		ret = check_label(start, state);
		if (ret < 0) { /*The method check_label already prints specified error*/
			return -1;
		}

		*label = start;

		/* Skip spaces after label and before operation */
		while (isspace(*p)) {
			p++;
		}
		if (*p == '\0') {
			fprintf(stderr, "Missing operation name after label, line %d\n", state -> line_number);
			return -1;
		}
		/* Read operation */
		start = p;
		while (isalnum(*p) || (*p == '.')) {
			++p;
		}
	}

	if (start == p) { /*If there was not any operation*/
		fprintf(stderr, "Unexpected character '%c'\n", *p);
		return -1;
	}
	/*There must be at least one space between operation and operands*/
	if (!isspace(*p) && !(*p == '\0')) {
		fprintf(stderr, "unexpected character '%c', line %d\n", *p, state -> line_number);
		return -1;
	}

	*operation = start;
	*(p++) = '\0';

	/* Rest of the line is operands */
	*operands = p;

	return 0;
}
