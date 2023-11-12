#include "thrvcc.h"
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// -cc1
static bool OptCC1;

// -###
static bool OptHashHashHash;

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
		// -###
		if (!strcmp(argv[i], "-###")) {
			OptHashHashHash = true;
			continue;
		}

		// -cc1
		if (!strcmp(argv[i], "-cc1")) {
			OptCC1 = true;
			continue;
		}

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

// run subprocess
static void run_subprocess(char **argv)
{
	// print all cli args of subprocess
	if (OptHashHashHash) {
		// program name
		fprintf(stderr, "%s", argv[0]);
		// program parameters
		for (int i = 1; argv[i]; i++)
			fprintf(stderr, " %s", argv[i]);
		// execute
		fprintf(stderr, "\n");
	}

	// fork-exec model
	// creates a copy of the current process, here a child process is created
	// return -1 error bit, return 0,success
	if (fork() == 0) {
		execvp(argv[0], argv);
		fprintf(stderr, "exec failed: %s: %s\n", argv[0],
			strerror(errno));
		_exit(1);
	}

	// parent process, wait
	int status;
	while (wait(&status) > 0)
		;

	// handle subprocess return value
	if (status != 0)
		exit(1);
}

// call cc1
// cc1 is thrvcc itself
// call itself
static void run_cc1(int argc, char **argv)
{
	// alloc more 10, casue new parameters
	char **args = calloc(argc + 10, sizeof(char *));
	// write parameters that pass in into program
	memcpy(args, argv, argc * sizeof(char *));
	// add '-cc1'
	args[argc++] = "-cc1";
	// call itself, pass in
	run_subprocess(args);
}

// compile C to asm
static void cc1(void)
{
	// parse input file, gen token stream
	struct Token *token = lexer_file(InputPath);

	// parse gen ast
	struct Obj_Var *prog = parse(token);

	FILE *out = open_file(TargetPath);
	// .file, file number, file name
	fprintf(out, ".file 1 \"%s\"\n", InputPath);
	codegen(prog, out);
}

// Compiler Driven Flow
//
// source file
//     ↓
// preprocess file
//     ↓
// cc1 compile to asm
//     ↓
// as compile to obj
//     ↓
// ld link to exec

int main(int argc, char *argv[])
{
	parse_args(argc, argv);

	// if -cc1
	// compile c to asm directly
	if (OptCC1) {
		cc1();
		return 0;
	}

	// call cc1 by default
	run_cc1(argc, argv);

	return 0;
}
