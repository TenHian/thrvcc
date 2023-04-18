#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

void error_out(char *fmt, ...);
void verror_at(char *location, char *fmt, va_list va);
void error_at(char *location, char *fmt, ...);
void error_token(struct Token *token, char *fmt, ...);
// token recognition
bool equal(struct Token *token, char *str);
struct Token *skip(struct Token *token, char *str);
// Lexical analysis
struct Token *lexer(char *formula);

enum NodeKind {
	ND_ADD,
	ND_SUB,
	ND_DIV,
	ND_MUL,
	ND_NEG,
	ND_EQ, // ==
	ND_NE, // !=
	ND_LT, // <
	ND_LE, // <=
	ND_NUM,
};

struct AstNode {
	enum NodeKind kind;
	struct AstNode *lhs;
	struct AstNode *rhs;
	int val;
};

struct AstNode *parse(struct Token *token);

void codegen(struct AstNode *node);