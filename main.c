#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

enum TokenKind {
	TK_PUNCT, // operator like "+" "-"
	TK_NUM,
	TK_EOF,
};

struct Token {
	enum TokenKind kind;
	struct Token *next;
	int val;
	char *location;
	int len;
};

static void error_out(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
	exit(1);
}

static bool equal(struct Token *token, char *str)
{
	return memcmp(token->location, str, token->len) == 0 &&
	       str[token->len] == '\0';
}

static struct Token *skip(struct Token *token, char *str)
{
	if (!equal(token, str))
		error_out("expect '%s'", str);
	return token->next;
}

static int get_number(struct Token *token)
{
	if (token->kind != TK_NUM)
		error_out("expect a number");
	return token->val;
}

static struct Token *new_token(enum TokenKind Kind, char *start, char *end)
{
	struct Token *token = calloc(1, sizeof(struct Token));
	token->kind = Kind;
	token->location = start;
	token->len = end - start;
	return token;
}

// Terminator Parser
static struct Token *token_parser(char *formula)
{
	struct Token head = {};
	struct Token *cur = &head;

	while (*formula) {
		// skip like whitespace, enter, tab
		if (isspace(*formula)) {
			++formula;
			continue;
		}

		// parse digit
		if (isdigit(*formula)) {
			cur->next = new_token(TK_NUM, formula, formula);
			cur = cur->next;
			const char *oldptr = formula;
			cur->val = strtoul(formula, &formula, 10);
			cur->len = formula - oldptr;
			continue;
		}

		// parse operator
		if (*formula == '+' || *formula == '-') {
			cur->next = new_token(TK_PUNCT, formula, formula + 1);
			cur = cur->next;
			++formula;
			continue;
		}

		error_out("invalid token: %c", *formula);
	}

	cur->next = new_token(TK_EOF, formula, formula);
	return head.next;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
		return 1;
	}

	struct Token *token = token_parser(argv[1]);
	printf("  .globl main\n");
	printf("main:\n");
	// parse first token, digit
	printf("  li a0, %d\n", get_number(token));
	token = token->next;

	// parse (opt + num)
	while (token->kind != TK_EOF) {
		if (equal(token, "+")) {
			token = token->next;
			printf("  addi a0, a0, %d\n", get_number(token));
			token = token->next;
			continue;
		}
		if (equal(token, "-")) {
			token = token->next;
			printf("  addi a0, a0, -%d\n", get_number(token));
			token = token->next;
		}
	}

	printf("  ret\n");

	return 0;
}