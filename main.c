#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
		return 1;
	}

	char *formula = argv[1];
	printf("  .globl main\n");
	printf("main:\n");
	printf("  li a0, %ld\n", strtol(formula, &formula, 10));
	while (*formula) {
		if (*formula == '+') {
			++formula;
			printf("  addi a0, a0, %ld\n",
			       strtol(formula, &formula, 10));
			continue;
		}
		if (*formula == '-') {
			++formula;
			printf("  addi a0, a0, -%ld\n",
			       strtol(formula, &formula, 10));
			continue;
		}
		// if other operator, error
		fprintf(stderr, "unexpected character: '%c'\n", *formula);
		return 1;
	}
	printf("  ret\n");

	return 0;
}