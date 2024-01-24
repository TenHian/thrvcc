#include "thrvcc.h"

static char *RVPath = "";

// -E
static bool OptE;
// -S
static bool OptS;

// -c
static bool OptC;

// -cc1
static bool OptCC1;

// -###
static bool OptHashHashHash;

// -o
static char *OptO;

// input file name
char *BaseFile;

// output file name
static char *OutputFile;

// Input file area
static struct StringArray InputPaths;

// temporary files
static struct StringArray TmpFiles;

// output thrvcc usage
static void usage(int status)
{
	fprintf(stderr, "thrvcc [ -o <path> ] <file>\n");
	exit(status);
}

// determines whether an option that takes one parameter has a single parameter
static bool take_arg(char *arg)
{
	return !strcmp(arg, "-o");
}

// parse the args passed in
static void parse_args(int argc, char **argv)
{
	// make sure that option that require one parameter, exist a parameter
	for (int i = 1; i < argc; i++)
		if (take_arg(argv[i]))
			if (!argv[++i])
				usage(1);

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
			// target file path
			OptO = argv[++i];
			continue;
		}

		// parse "-oXXX" args
		if (!strncmp(argv[i], "-o", 2)) {
			// target file path
			OptO = argv[i] + 2;
			continue;
		}

		// parse -S
		if (!strcmp(argv[i], "-S")) {
			OptS = true;
			continue;
		}

		// parse -c
		if (!strcmp(argv[i], "-c")) {
			OptC = true;
			continue;
		}

		// parse -E
		if (!strcmp(argv[i], "-E")) {
			OptE = true;
			continue;
		}

		// parse -cc1-input
		if (!strcmp(argv[i], "-cc1-input")) {
			BaseFile = argv[++i];
			continue;
		}

		// parse -cc1-output
		if (!strcmp(argv[i], "-cc1-output")) {
			OutputFile = argv[++i];
			continue;
		}

		// parse "-" args
		if (argv[i][0] == '-' && argv[i][1] != '\0')
			error_out("unknown argument: %s", argv[i]);

		// others, input file path
		str_array_push(&InputPaths, argv[i]);
	}
	// if InputPath not exist, error
	if (InputPaths.len == 0)
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

// determine if the string P ends with the string Q
static bool ends_with(char *P, char *Q)
{
	int len1 = strlen(P);
	int len2 = strlen(Q);
	return (len1 >= len2) && !strcmp(P + len1 - len2, Q);
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

	if (input) {
		args[argc++] = "-cc1-input";
		args[argc++] = input;
	}

	if (output) {
		args[argc++] = "-cc1-output";
		args[argc++] = output;
	}

	// call itself, pass in
	run_subprocess(args);
}

// when -E, print all terminators
static void print_tokens(struct Token *token)
{
	// default output stdout
	FILE *out = open_file(OptO ? OptO : "-");

	// reg line numbers
	int line = 1;
	for (; token->kind != TK_EOF; token = token->next) {
		if (line > 1 && token->at_bol)
			fprintf(out, "\n");
		if (token->has_space && !token->at_bol)
			fprintf(out, " ");
		fprintf(out, "%.*s", token->len, token->location);
		line++;
	}
	fprintf(out, "\n");
}

// compile C to asm
static void cc1(void)
{
	// parse input file, gen token stream
	struct Token *token = lexer_file(BaseFile);
	// if generate terminator stream failed, error out file
	if (!token)
		error_out("%s: %s", BaseFile, strerror(errno));

	// preprocess
	token = preprocesser(token);

	// if -E, print C code after preprocess
	if (OptE) {
		print_tokens(token);
		return;
	}

	// parse gen ast
	struct Obj_Var *prog = parse(token);

	FILE *out = open_file(OutputFile);
	codegen(prog, out);
}

// call assembler
static void assembler(char *input, char *output)
{
#ifdef RVPATH
	char *as = format("%s/bin/riscv64-unknown-linux-gnu-as", RVPath);
#else
	char *as = "as";
#endif /* ifdef RVPATH */
	char *cmd[] = { as, "-c", input, "-o", output, NULL };
	run_subprocess(cmd);
}

static char *find_file(char *pattern)
{
	char *path = NULL;
	glob_t buf = {};
	glob(pattern, 0, NULL, &buf);
	if (buf.gl_pathc > 0)
		path = strdup(buf.gl_pathv[buf.gl_pathc - 1]);
	globfree(&buf);
	return path;
}

static bool file_exists(char *path)
{
	struct stat st;
	return !stat(path, &st);
}

static char *find_lib_path(void)
{
#ifdef RVPATH
	if (file_exists(format("%s/sysroot/usr/lib/crti.o", RVPath)))
		return format("%s/sysroot/usr/lib", RVPath);
#else
	if (file_exists("/usr/lib/riscv64-linux-gnu/crti.o"))
		return "/usr/lib/riscv64-linux-gnu";
	if (file_exists("/usr/lib64/crti.o"))
		return "/usr/lib64";

#endif /* ifdef RVPATH */

	error_out("library path is not find");
	return NULL;
}

static char *find_gcc_lib_path(void)
{
#ifdef RVPATH
	char *path = find_file(format(
		"%s/lib/gcc/riscv64-unknown-linux-gnu/*/crtbegin.o", RVPath));
	if (path)
		return dirname(path);
#endif /* ifdef RVPATH */

	char *paths[] = {
		"/usr/lib/gcc/riscv64-linux-gnu/*/crtbegin.o ",
		//Gentoo
		"/usr/lib/gcc/riscv64-pc-linux-gnu/*/crtbegin.o",
		// Fedora
		"/usr/lib/gcc/riscv64-redhat-linux/*/crtbegin.o",
	};

	for (int i = 0; i < sizeof(paths) / sizeof(*paths); i++) {
		char *path = find_file(paths[i]);
		if (path)
			return dirname(path);
	}

	error_out("gcc library path is not found");
	return NULL;
}

static void run_linker(struct StringArray *inputs, char *output)
{
	// args need be passed to ld
	struct StringArray arr = {};

#ifdef RVPATH
	char *ld = format("%s/bin/riscv64-unknown-linux-gnu-ld", RVPath);
#else
	char *ld = "ld";
#endif /* ifdef RVPATH */
	str_array_push(&arr, ld);
	str_array_push(&arr, "-o");
	str_array_push(&arr, output);
	str_array_push(&arr, "-m");
	str_array_push(&arr, "elf64lriscv");
	str_array_push(&arr, "-dynamic-linker");

#ifdef RVPATH
	char *lp64d =
		format("%s/sysroot/lib/ld-linux-riscv64-lp64d.so.1", RVPath);
#else
	char *lp64d = "/lib/ld-linux-riscv64-lp64d.so.1";
#endif /* ifdef RVPATH */
	str_array_push(&arr, lp64d);

	char *lib_path = find_lib_path();
	char *gcc_lib_path = find_gcc_lib_path();

	str_array_push(&arr, format("%s/crt1.o", lib_path));
	str_array_push(&arr, format("%s/crti.o", lib_path));
	str_array_push(&arr, format("%s/crtbegin.o", gcc_lib_path));
	str_array_push(&arr, format("-L%s", gcc_lib_path));
	str_array_push(&arr, format("-L%s", lib_path));
	str_array_push(&arr, format("-L%s/..", lib_path));
#ifdef RVPATH
	str_array_push(&arr, format("-L%s/sysroot/usr/lib64", RVPath));
	str_array_push(&arr, format("-L%s/sysroot/lib64", RVPath));
	str_array_push(&arr, format("-L%s/sysroot/usr/lib/riscv64-linux-gnu",
				    RVPath));
	str_array_push(&arr, format("-L%s/sysroot/usr/lib/riscv64-pc-linux-gnu",
				    RVPath));
	str_array_push(&arr, format("-L%s/sysroot/usr/lib/riscv64-redhat-linux",
				    RVPath));
	str_array_push(&arr, format("-L%s/sysroot/usr/lib", RVPath));
	str_array_push(&arr, format("-L%s/sysroot/lib", RVPath));
#else
	str_array_push(&arr, "-L/usr/lib64");
	str_array_push(&arr, "-L/lib64");
	str_array_push(&arr, "-L/usr/lib/riscv64-linux-gnu");
	str_array_push(&arr, "-L/usr/lib/riscv64-pc-linux-gnu");
	str_array_push(&arr, "-L/usr/lib/riscv64-redhat-linux");
	str_array_push(&arr, "-L/usr/lib");
	str_array_push(&arr, "-L/lib");
#endif /* ifdef RVPATH */

	for (int i = 0; i < inputs->len; i++)
		str_array_push(&arr, inputs->data[i]);

	str_array_push(&arr, "-lc");
	str_array_push(&arr, "-lgcc");
	str_array_push(&arr, "--as-needed");
	str_array_push(&arr, "-lgcc_s");
	str_array_push(&arr, "--no-as-needed");
	str_array_push(&arr, format("%s/crtend.o", gcc_lib_path));
	str_array_push(&arr, format("%s/crtn.o", lib_path));
	str_array_push(&arr, NULL);

	run_subprocess(arr.data);
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
#ifdef RVPATH
	RVPath = getenv("RISCV");
	if (RVPath == NULL)
		error_out("riscv not find");
#endif /* ifdef MACRO */

	// call cleanup() when program end
	atexit(cleanup);

	parse_args(argc, argv);

	// if -cc1
	// compile c to asm directly
	if (OptCC1) {
		cc1();
		return 0;
	}

	// currently, it is not possible to output multiple input files
	// to a single file after -c -S -E
	if (InputPaths.len > 1 && OptO && (OptC || OptS || OptE))
		error_out(
			"cannot specify '-o' with '-c', '-S' or '-E' with multiple files");

	// ld's args
	struct StringArray ld_args = {};

	// iterate though all input file
	for (int i = 0; i < InputPaths.len; i++) {
		// read input file
		char *input = InputPaths.data[i];

		char *output;
		if (OptO)
			output = OptO;
		else if (OptS)
			output = replace_extn(input, ".s");
		else
			output = replace_extn(input, ".o");

		// .o
		if (ends_with(input, ".o")) {
			str_array_push(&ld_args, input);
			continue;
		}

		// .s
		if (ends_with(input, ".s")) {
			if (!OptS)
				assembler(input, output);
			continue;
		}

		// .c
		if (!ends_with(input, ".c") && strcmp(input, "-"))
			error_out("unknown file extension: %s", input);

		if (OptE) {
			run_cc1(argc, argv, input, NULL);
			continue;
		}

		// if '-S', call cc1
		if (OptS) {
			run_cc1(argc, argv, input, output);
			continue;
		}

		// compile and assemble
		if (OptC) {
			char *tmp = create_tmpfile();
			run_cc1(argc, argv, input, tmp);
			assembler(tmp, output);
			continue;
		}

		// else call cc1 and as
		char *tmp1 = create_tmpfile();
		char *tmp2 = create_tmpfile();
		run_cc1(argc, argv, input, tmp1);
		assembler(tmp1, tmp2);
		str_array_push(&ld_args, tmp2);
		continue;
	}

	if (ld_args.len > 0)
		run_linker(&ld_args, OptO ? OptO : "a.out");

	return 0;
}
