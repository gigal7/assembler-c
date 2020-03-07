#include "assembler.h"
#include "symtable.h"

#include <string.h>
#include <stdio.h>

/*This method initialize the state of the assembler
 * returns 0 in case of success and -1 otherwise*/
int init_state(assembler_state_t *state, const char *filename)
{
	state->IC = 0;
	state->DC = 0;
	symtab_init(&state->symbols);
	state->filename = filename;
	return 0;
}

/*This method clean (free) the state of the assembler*/
void cleanup_state(assembler_state_t *state)
{
	symtab_free(&state->symbols);
}

/*This method does the first and only pass of transformation of the assembler file to 32 special base
 * returns 0 in case of success and -1 otherwise */
int generate_code_and_data(assembler_state_t *state)
{
	char *label, *operation, *operands;
	operation_info_t *opinfo;
	char line[MAX_LINE_LENGTH];
	FILE *asfile;
	char *p;
	int ret;
	int ic, dc;
	int error_flag;

	/* open .as file */
	asfile = open_file_with_ext(state->filename, "as", "r");
	if (asfile == NULL) {
		return -1;
	}

	state-> line_number = 0;

	error_flag = 0;

	while (fgets(line, MAX_LINE_LENGTH, asfile) != NULL) {

		state-> line_number++;
		/* Remove trailing '\n' and everything after ';' */
		p = strpbrk(line, "\n;");
		if (p != NULL) {
			*p = '\0';
		}

		ret = tokenize_line(line, &label, &operation, &operands, state);
		if (ret < 0) {
			error_flag = ret; /*Lexical analyzing did not went well*/
			continue;
		}

		if (operation == NULL) {
			continue;
		}

		opinfo = find_operation(operation);
		if (opinfo == NULL) {
			fprintf(stderr, "Missing operation '%s', in line '%d'\n", operation, state -> line_number );
			error_flag = -1;
			continue;
		}

		ic = state->IC;
		dc = state->DC;
		ret = opinfo->parse(opinfo, state, operands);
		if (ret < 0) {
			error_flag = ret;
			continue;
		}

		if (label != NULL) { /*There is a label*/
			symtab_new_label(&state->symbols, label, opinfo->symtype, ic, dc);
			if (ret < 0) {
				error_flag = ret;
				continue;
			}
		}
	}

	fclose(asfile);
	return error_flag;
}

/*This method write the obj file of the assembler
 * returns 0 in case of success and -1 otherwise */
int write_object(assembler_state_t *state)
{
	char addr_base32[3], word_base32[3];
	int i, address, word;
	FILE *obfile;

	/* open .ob file */
	obfile = open_file_with_ext(state->filename, "ob", "w");
	if (obfile == NULL) {
		return -1;
	}

	address = ASSEMBLY_CODE_START_ADDRESS;

	for (i = 0; i < state->IC; i++) {
		word = state->code[i];
		to_base32(address, addr_base32);
		to_base32(word, word_base32);
		fprintf(obfile, "%s %s\n", addr_base32, word_base32);
		address++;
	}

	for (i = 0; i < state->DC; i++) {
		word = state->data[i];
		to_base32(address, addr_base32);
		to_base32(word, word_base32);
		fprintf(obfile, "%s %s\n", addr_base32, word_base32);
		address++;
	}

	fclose(obfile);

	return 0;
}

/* Assemble the given <filename>.as to <filename>.obj, <filename>.ext, <filename>.ent.
 * returns 0 in case of success and -1 otherwise */
int assemble_one_file(const char *filename)
{
	assembler_state_t state;
	int ret;

	ret = init_state(&state, filename);
	if(ret < 0){
		return ret;
	}

	ret = generate_code_and_data(&state);
	if(ret < 0) {
		cleanup_state(&state);
		return ret;
	}

	ret = symtab_update_relocations_and_write(&state.symbols, &state);
	if(ret < 0) {
		cleanup_state(&state);
		return ret;
	}

	ret = write_object(&state);
	if(ret < 0) {
		cleanup_state(&state);
		return ret;
	}

	cleanup_state(&state);
	return 0;
}

/*This method is the main of this project - go through all the files .as given in command line
 * and returns 0 in case of success making target files - ent, ext, obj and -1 otherwise */
int main(int argc, char* argv[])
{
	int ret;
	int i;

	/* Check if no arguments provided */
	if (argc == 1) {
		fprintf(stderr, "Expected an argument\n");
		return 1;
	}

	for (i = 1; i < argc; i++){
		ret = assemble_one_file(argv[i]);
		if (ret < 0) { /*assemble_one_file already gives specified error*/
			break;
		}
	}

	return ret;
}
