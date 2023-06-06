#include "thrvcc.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static char *CurLexStream; // reg the cur lexing stream
static char *SourceFile; // current lexing file

void error_out(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
	exit(1);
}

// output error message, exit.
// foo.c:10: x = y + 1;
//               ^ <error message>
void verror_at(char *location, char *fmt, va_list va)
{
	// find line that in location
	char *line = location;
	// <line> decrements to the beginning of the current line
	// line<CurLexStream, determines if the file is read at the beginning
	// line[-1] ! = '\n', whether the previous character of the Line string \
	// is a line break (at the end of the previous line)
	while (CurLexStream < line && line[-1] != '\n')
		line--;

	// end incremental line break to end of line
	char *end = location;
	while (*end != '\n')
		end++;

	// get line number
	int line_no = 1;
	for (char *p = CurLexStream; p < line; p++)
		// if '\n', line_no +1
		if (*p == '\n')
			line_no++;

	// output filename:error line
	// ident reg the char number that output
	int ident = fprintf(stderr, "%s:%d: ", SourceFile, line_no);
	// out all char in line, exclude '\n'
	fprintf(stderr, "%.*s\n", (int)(end - line), line);

	int pos = location - line + ident;
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

// consume the token
bool consume(struct Token **rest, struct Token *token, char *str)
{
	// exist
	if (equal(token, str)) {
		*rest = token->next;
		return true;
	}
	// not exist
	*rest = token;
	return false;
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

// determine the first letter of the identifier
// a-z A-Z _
static bool is_ident_1(char c)
{
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// determine the other letter of the identifier
// a-z A-Z 0-9 _
static bool is_ident_2(char c)
{
	return is_ident_1(c) || ('0' <= c && c <= '9');
}

// return one bit turns hex to dec
// hex digit = [0-9a-fA-F]
static int from_hex(char c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('a' <= c && c <= 'f')
		return c - 'a' + 10;
	return c - 'A' + 10;
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

// determine if it is a keyword
static bool is_keyword(struct Token *token)
{
	// keyword list
	static char *KeyWords[] = { "return", "if",  "else",   "for",
				    "while",  "int", "sizeof", "char" };

	for (int i = 0; i < sizeof(KeyWords) / sizeof(*KeyWords); ++i) {
		if (equal(token, KeyWords[i]))
			return true;
	}
	return false;
}

// read escaped characters
static int read_escaped_char(char **new_pos, char *p)
{
	if ('0' <= *p && *p <= '7') {
		// Read an octal number, not longer than three digits
		// \abc = (a*8+b)*8+c
		int c = *p++ - '0';
		if ('0' <= *p && *p <= '7') {
			c = (c << 3) + (*p++ - '0');
			if ('0' <= *p && *p <= '7')
				c = (c << 3) + (*p++ - '0');
		}
		*new_pos = p;
		return c;
	}
	if (*p == 'x') {
		p++;
		// Determine if the number is a hexadecimal number
		if (!isxdigit(*p))
			error_at(p, "invalid hex escape sequence");

		int c = 0;
		// Reads one or more hexadecimal digits
		// \xWXYZ = ((W*16+X)*16+Y)*16+Z
		for (; isxdigit(*p); p++)
			c = (c << 4) + from_hex(*p);
		*new_pos = p;
		return c;
	}

	*new_pos = p + 1;
	switch (*p) {
	case 'a': // ring the bell, alarm
		return '\a';
	case 'b': // backspace
		return '\b';
	case 't': // horizontal Tabs
		return '\t';
	case 'n': // line break
		return '\n';
	case 'v': // vertical Tabs
		return '\v';
	case 'f': // page break
		return '\f';
	case 'r': // enter
		return '\r';
	// GUN C extensions
	case 'e': // escape character
		return 27;
	default:
		return *p;
	}
}

static char *string_literal_end(char *p)
{
	char *start = p;
	for (; *p != '"'; p++) {
		if (*p == '\n' || *p == '\0')
			error_at(start, "unclosed string literal");
		if (*p == '\\')
			p++;
	}
	return p;
}

// read string literals
static struct Token *read_string_literal(char *start)
{
	// Read right quotes to string literals
	char *end = string_literal_end(start + 1);
	// Define a Buf with the number of characters in the string literal + 1
	// Use it to store the maximum number of bits of string literals
	char *buf = calloc(1, end - start);
	// Actual number of character bits, one escape character is 1 bit
	int len = 0;

	// identify all char in string that not ' " '
	for (char *p = start + 1; p < end;)
		if (*p == '\\') {
			buf[len++] = read_escaped_char(&p, p + 1);
		} else {
			buf[len++] = *p++;
		}

	// Token needs to contain a string literal with double quotes here
	struct Token *token = new_token(TK_STR, start, end + 1);
	// Add a bit to \0
	token->type = array_of(TyChar, len + 1);
	token->str = buf;
	return token;
}

// convert the terminator named "return" to KEYWORD
static void convert_keyword(struct Token *token)
{
	for (struct Token *t = token; t->kind != TK_EOF; t = t->next) {
		if (is_keyword(t))
			t->kind = TK_KEYWORD;
	}
}

// Terminator Parser
struct Token *lexer(char *filename, char *formula)
{
	SourceFile = filename;
	CurLexStream = formula;
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

		// parse string literal
		if (*formula == '"') {
			cur->next = read_string_literal(formula);
			cur = cur->next;
			formula += cur->len;
			continue;
		}

		// parse identifiers or keyword
		if (is_ident_1(*formula)) {
			char *start = formula;
			do {
				++formula;
			} while (is_ident_2(*formula));
			cur->next = new_token(TK_IDENT, start, formula);
			cur = cur->next;
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
	// turn all of keyword that terminator into keyword
	convert_keyword(head.next);
	return head.next;
}

// return file content
static char *read_file(char *path)
{
	FILE *fp;
	if (strcmp(path, "-") == 0) {
		// if filename = '-', read frome input
		fp = stdin;
	} else {
		fp = fopen(path, "r");
		if (!fp)
			error_out("cannot open %s: %s", path, strerror(errno));
	}

	// the string that need to return
	char *buf;
	size_t buf_len;
	FILE *out = open_memstream(&buf, &buf_len);

	// read the whole file
	while (true) {
		char buf2[4096];
		int N = fread(buf2, 1, sizeof(buf2), fp);
		if (N == 0)
			break;
		fwrite(buf2, 1, N, out);
	}

	// read file over
	if (fp != stdin)
		fclose(fp);

	// refulsh ouput stream
	fflush(out);
	// make sure end at '\n'
	if (buf_len == 0 || buf[buf_len - 1] != '\n')
		fputc('\n', out);
	fputc('\0', out);
	fclose(out);
	return buf;
}

// file lexing
struct Token *lexer_file(char *path)
{
	return lexer(path, read_file(path));
}
