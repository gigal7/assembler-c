/*name: Gal Girad ID 307901199*/
#include "defs.h"
#include "assembler.h"

#include <stdlib.h>

/*This method opens a file and returns a pointer to it
 * if failed to open it returns null*/
FILE *open_file_with_ext(const char *filename, const char *ext, const char *mode)
{
	char filename_with_ext[MAX_PATH];
	FILE *f;


	sprintf(filename_with_ext, "%s.%s", filename, ext);
	f = fopen(filename_with_ext, mode);
	if (f == NULL) {
		fprintf(stderr, "Cannot open file %s for reading\n",
				filename_with_ext);
	}
	return f;
}
/*This method converts a number to special base 32*/
void to_base32(int x, char *str)
{
	const char base32[]="!@#$%^&*<>abcdefghijklmnopqrstuv";

	str[0] = base32[x / 32];
	str[1] = base32[x % 32];
	str[2] = '\0';
}

/*My version of atoi that handle errors*/
int my_atoi(assembler_state_t *state, char *number_str, int *number)
{
	char *endptr;

	*number = strtol(number_str, &endptr, 10);

	/* check if all string is converted */
	if ((*endptr != '\0') || (endptr == number_str)) {
		fprintf(stderr, "Invalid numeric value, line %d\n", state->line_number);
		return -1; /* Failed */
	} else{
		return 0; /* Success */
	}
}
