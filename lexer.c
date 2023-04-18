#include "thrvcc.h"

static char *InputArgv; // reg the argv[1]

void error_out(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
	exit(1);
}

void verror_at(char *location, char *fmt, va_list va)
{
	fprintf(stderr, "%s\n", InputArgv);

	int pos = location - InputArgv;
	fprintf(stderr, "%*s", pos, " ");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
}

void error_at(char *location, char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	verror_at(location, fmt, va);
	exit(1);
}

void error_token(struct Token *token, char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	verror_at(token->location, fmt, va);
	exit(1);
}

bool equal(struct Token *token, char *str)
{
	return memcmp(token->location, str, token->len) == 0 &&
	       str[token->len] == '\0';
}

struct Token *skip(struct Token *token, char *str)
{
	if (!equal(token, str))
		error_token(token, "expect '%s'", str);
	return token->next;
}

int get_number(struct Token *token)
{
	if (token->kind != TK_NUM)
		error_token(token, "expect a number");
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

// Determine if Str starts with SubStr
static bool is_start_with(char *str, char *substr)
{
	return strncmp(str, substr, strlen(substr)) == 0;
}

static int read_punct(char *op_str)
{
	// case binary operator
	if (is_start_with(op_str, "==") || is_start_with(op_str, "!=") ||
	    is_start_with(op_str, "<=") || is_start_with(op_str, ">="))
		return 2;
	// case unary operator
	return ispunct(*op_str) ? 1 : 0;
}

// Terminator Parser
struct Token *lexer(char *formula)
{
	InputArgv = formula;
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
		int punct_len = read_punct(formula);
		if (punct_len) {
			cur->next = new_token(TK_PUNCT, formula,
					      formula + punct_len);
			cur = cur->next;
			formula += punct_len;
			continue;
		}

		error_at(formula, "invalid token");
	}

	cur->next = new_token(TK_EOF, formula, formula);
	return head.next;
}