#include "thrvcc.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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
void verror_at(int line_no, char *location, char *fmt, va_list va)
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

	// output filename:error line
	// ident reg the char number that output
	int ident = fprintf(stderr, "%s:%d: ", SourceFile, line_no);
	// out all char in line, exclude '\n'
	fprintf(stderr, "%.*s\n", (int)(end - line), line);

	int pos = location - line + ident;
	fprintf(stderr, "%*s", pos, "");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
}

void error_at(char *location, char *fmt, ...)
{
	int line_no = 1;
	for (char *p = CurLexStream; p < location; p++)
		if (*p == '\n')
			line_no++;

	va_list va;
	va_start(va, fmt);
	verror_at(line_no, location, fmt, va);
	exit(1);
}

void error_token(struct Token *token, char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	verror_at(token->line_no, token->location, fmt, va);
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
	static char *kw[] = {
		"==", "!=", "<=", ">=", "+=",  "-=",  "*=", "/=",
		"%=", "&=", "|=", "^=", "<<=", ">>=", "++", "--",
		"->", "&&", "||", "<<", ">>",  "...",
	};
	for (int i = 0; i < sizeof(kw) / sizeof(*kw); ++i) {
		if (is_start_with(op_str, kw[i]))
			return strlen(kw[i]);
	}

	// case unary operator
	return ispunct(*op_str) ? 1 : 0;
}

// determine if it is a keyword
static bool is_keyword(struct Token *token)
{
	// keyword list
	static char *KeyWords[] = {
		"return",    "if",	 "else",       "for",
		"while",     "int",	 "sizeof",     "char",
		"struct",    "union",	 "long",       "short",
		"void",	     "typedef",	 "_Bool",      "enum",
		"static",    "goto",	 "break",      "continue",
		"switch",    "case",	 "default",    "extern",
		"_Alignof",  "_Alignas", "do",	       "signed",
		"unsigned",  "const",	 "volatile",   "auto",
		"register",  "restrict", "__restrict", "__restrict__",
		"_Noreturn", "float",	 "double",
	};

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

// read character literals
static struct Token *read_char_literal(char *start)
{
	char *p = start + 1;
	// parse character case '\0'
	if (*p == '\0')
		error_at(start, "unclosed char literal");

	// parse character
	char c;
	// escaped characters
	if (*p == '\\')
		c = read_escaped_char(&p, p + 1);
	else
		c = *p++;

	// strchr() returns a string starting with ', or NULL if none
	char *end = strchr(p, '\'');
	if (!end)
		error_at(p, "unclosed char literal");

	// Constructs a NUM terminator with the value of C
	struct Token *token = new_token(TK_NUM, start, end + 1);
	token->val = c;
	token->type = TyInt;
	return token;
}

// read integer number literal
static struct Token *read_int_literal(char *start)
{
	char *p = start;

	// read binary, octal, decimal, hexadecimal
	// default decimal
	int base = 10;
	// compare the first 2 characters of two strings, \
	// ignore case, and determine if they are numbers
	if (!strncasecmp(p, "0x", 2) && isxdigit(p[2])) {
		// hexadecimal
		p += 2;
		base = 16;
	} else if (!strncasecmp(p, "0b", 2) && (p[2] == '0' || p[2] == '1')) {
		// binary
		p += 2;
		base = 2;
	} else if (*p == '0') {
		// octal
		base = 8;
	}

	// convert string to base binary number
	int64_t val = strtoul(p, &p, base);
	// read 'U' 'L' 'LL' suffix
	bool L = false;
	bool U = false;
	// LLU
	if (is_start_with(p, "LLU") || is_start_with(p, "LLu") ||
	    is_start_with(p, "llU") || is_start_with(p, "llu") ||
	    is_start_with(p, "ULL") || is_start_with(p, "Ull") ||
	    is_start_with(p, "uLL") || is_start_with(p, "ull")) {
		p += 3;
		L = U = true;
	} else if (!strncasecmp(p, "lu", 2) || !strncasecmp(p, "ul", 2)) {
		// LU
		p += 2;
		L = U = true;
	} else if (is_start_with(p, "LL") || is_start_with(p, "ll")) {
		// LL
		p += 2;
		L = true;
	} else if (*p == 'L' || *p == 'l') {
		// L
		p++;
		L = true;
	} else if (*p == 'U' || *p == 'u') {
		// U
		p++;
		U = true;
	}

	// infer the type and use the type that holds the current value
	struct Type *type;
	if (base == 10) {
		if (L && U)
			type = TyULong;
		else if (L)
			type = TyLong;
		else if (U)
			type = (val >> 32) ? TyULong : TyUInt;
		else
			type = (val >> 31) ? TyLong : TyInt;
	} else {
		if (L && U)
			type = TyULong;
		else if (L)
			type = (val >> 63) ? TyULong : TyLong;
		else if (U)
			type = (val >> 32) ? TyULong : TyUInt;
		else if (val >> 63)
			type = TyULong;
		else if (val >> 32)
			type = TyLong;
		else if (val >> 31)
			type = TyUInt;
		else
			type = TyInt;
	}
	// construct num terminator
	struct Token *token = new_token(TK_NUM, start, p);
	token->val = val;
	token->type = type;
	return token;
}

// read num
static struct Token *read_number(char *start)
{
	// attempt to parse integer integral constant
	struct Token *token = read_int_literal(start);
	// no suffix 'e' or 'f', integer
	if (!strchr(".eEfF", start[token->len]))
		return token;

	// if not integer, must be float
	char *end;
	double val = strtod(start, &end);

	// handle float suffix
	struct Type *type;
	if (*end == 'f' || *end == 'F') {
		type = TyFloat;
		end++;
	} else if (*end == 'l' || *end == 'L') {
		type = TyDouble;
		end++;
	} else {
		type = TyDouble;
	}
	// construct float terminator
	token = new_token(TK_NUM, start, end);
	token->fval = val;
	token->type = type;
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

// add line number for all token
static void add_line_numbers(struct Token *token)
{
	char *p = CurLexStream;
	int line_number = 1;

	do {
		if (p == token->location) {
			token->line_no = line_number;
			token = token->next;
		}
		if (*p == '\n')
			line_number++;
	} while (*p++);
}

// Terminator Parser
struct Token *lexer(char *filename, char *formula)
{
	SourceFile = filename;
	CurLexStream = formula;
	struct Token head = {};
	struct Token *cur = &head;

	while (*formula) {
		// skip comment lines
		if (is_start_with(formula, "//")) {
			formula += 2;
			while (*formula != '\n')
				formula++;
			continue;
		}

		//skip comment blocks
		if (is_start_with(formula, "/*")) {
			// find first "*/"
			char *block_end = strstr(formula + 2, "*/");
			if (!block_end)
				error_at(formula, "unclosed block comment");
			formula = block_end + 2;
			continue;
		}

		// skip like whitespace, enter, tab
		if (isspace(*formula)) {
			++formula;
			continue;
		}

		// parse digit (integer or float)
		if (isdigit(*formula) ||
		    (*formula == '.' && isdigit(formula[1]))) {
			// read number literal
			cur->next = read_number(formula);
			// pointer forward
			cur = cur->next;
			formula += cur->len;
			continue;
		}

		// parse string literal
		if (*formula == '"') {
			cur->next = read_string_literal(formula);
			cur = cur->next;
			formula += cur->len;
			continue;
		}

		// parse char literal
		if (*formula == '\'') {
			cur->next = read_char_literal(formula);
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
	// add line number for all token
	add_line_numbers(head.next);
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
