
all: assembler

assembler: assembler.c parsing.c symtable.c symtable.h defs.h assembler.h Makefile util.c
	gcc -g -Wall -ansi -pedantic assembler.c parsing.c symtable.c util.c -o assembler
