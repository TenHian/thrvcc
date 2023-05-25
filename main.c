#include "thrvcc.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		error_out("%s: invalid number of arguments\n", argv[0]);
	}

	// parse argv[1], gen token stream
	struct Token *token = lexer(argv[1]);

	// parse gen ast
	struct Function *prog = parse(token);

	codegen(prog);

	return 0;
}