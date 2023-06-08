#include "thrvcc.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Target file path
static char *TargetPath;

// Input file path
static char *InputPath;

// output thrvcc usage
static void usage(int status)
{
	fprintf(stderr, "thrvcc [ -o <path> ] <file>\n");
	exit(status);
}

// parse the args passed in
static void parse_args(int argc, char **argv)
{
	// Iterate over all parameters passed into the program
	for (int i = 1; i < argc; i++) {
		// if --help, output usage()
		if (!strcmp(argv[i], "--help"))
			usage(0);

		// parse "-o XXX" args
		if (!strcmp(argv[i], "-o")) {
			// target file not exist, error
			if (!argv[++i])
				usage(1);
			// target file path
			TargetPath = argv[i];
			continue;
		}

		// parse "-oXXX" args
		if (!strncmp(argv[i], "-o", 2)) {
			// target file path
			TargetPath = argv[i] + 2;
			continue;
		}

		// parse "-" args
		if (argv[i][0] == '-' && argv[i][1] != '\0')
			error_out("unknown argument: %s", argv[i]);

		// others, input file path
		InputPath = argv[i];
	}
	// if InputPath not exist, error
	if (!InputPath)
		error_out("no input files");
}

// open the file need to write
static FILE *open_file(char *path)
{
	if (!path || strcmp(path, "-") == 0)
		return stdout;

	// open file in w mode
	FILE *out = fopen(path, "w");
	if (!out)
		error_out("cannot open output file: %s: %s", path,
			  strerror(errno));
	return out;
}

int main(int argc, char *argv[])
{
	// parse args passed in
	parse_args(argc, argv);

	// parse input file, gen token stream
	struct Token *token = lexer_file(InputPath);

	// parse gen ast
	struct Obj_Var *prog = parse(token);

	FILE *out = open_file(TargetPath);
	// .file, file number, file name
	fprintf(out, ".file 1 \"%s\"\n", InputPath);
	codegen(prog, out);

	return 0;
}
