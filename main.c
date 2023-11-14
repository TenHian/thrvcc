#include "thrvcc.h"
#include <errno.h>
#include <libgen.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// [attention]
// if you are cross-compiling, change this path to the path corresponding to ricsv toolchain
// must be an absulte path
// else leave it empty
static char *RVPath = "/usr";

// -S
static bool OptS;

// -cc1
static bool OptCC1;

// -###
static bool OptHashHashHash;

// Target file path
static char *TargetPath;

// Input file path
static char *InputPath;

// temporary files
static struct StringArray TmpFiles;

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

		// parse -S
		if (!strcmp(argv[i], "-S")) {
			OptS = true;
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

// replace file extern name
static char *replace_extn(char *tmpl, char *extn)
{
	char *file_name = basename(strdup(tmpl));
	char *dot = strrchr(file_name, '.');
	if (dot)
		*dot = '\0';
	return format("%s%s", file_name, extn);
}

// cleanup temporary files
static void cleanup(void)
{
	for (int i = 0; i < TmpFiles.len; i++)
		unlink(TmpFiles.data[i]);
}

// create temporary files
static char *create_tmpfile(void)
{
	char *path = strdup("/tmp/thrvcc-XXXXXX");
	int fd = mkstemp(path);
	if (fd == -1)
		error_out("mkstemp failed: %s", strerror(errno));
	close(fd);

	str_array_push(&TmpFiles, path);
	return path;
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
static void run_cc1(int argc, char **argv, char *input, char *output)
{
	// alloc more 10, casue new parameters
	char **args = calloc(argc + 10, sizeof(char *));
	// write parameters that pass in into program
	memcpy(args, argv, argc * sizeof(char *));
	// add '-cc1'
	args[argc++] = "-cc1";

	if (input)
		args[argc++] = input;

	if (output) {
		args[argc++] = "-o";
		args[argc++] = output;
	}

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

// call assembler
static void assembler(char *input, char *output)
{
	char *as = strlen(RVPath) ? format("%s/bin/riscv64-elf-as", RVPath) :
				    "as";
	char *cmd[] = { as, "-c", input, "-o", output, NULL };
	run_subprocess(cmd);
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
	// call cleanup() when program end
	atexit(cleanup);

	parse_args(argc, argv);

	// if -cc1
	// compile c to asm directly
	if (OptCC1) {
		cc1();
		return 0;
	}

	char *output;
	if (TargetPath)
		output = TargetPath;
	else if (OptS)
		output = replace_extn(InputPath, ".s");
	else
		output = replace_extn(InputPath, ".o");

	// if '-S', call cc1
	if (OptS) {
		run_cc1(argc, argv, InputPath, output);
		return 0;
	}

	// else call cc1 and as
	char *tmp_file = create_tmpfile();
	run_cc1(argc, argv, InputPath, tmp_file);
	assembler(tmp_file, output);

	return 0;
}
