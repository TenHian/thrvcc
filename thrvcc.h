// use POSIX.1 standard
// use func strndup()

#define _POSIX_C_SOURCE 200909L

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

enum TokenKind {
	TK_IDENT, // identifiers
	TK_PUNCT, // operator like "+" "-"
	TK_KEYWORD, // keyword
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
	ND_ASSIGN,
	ND_RETURN, // return
	ND_IF, // if
	ND_FOR, // for or while
	ND_BLOCK, // code block
	ND_VAR, // variable
	ND_NUM,
	ND_EXPR_STMT, // express statement
};

struct Local_Var {
	struct Local_Var *next; // point to next local_var
	char *name; // local_var name
	int offset; // register fp offset
};

struct Function {
	struct AstNode *body; // func body
	struct Local_Var *locals; // local variables
	int stack_size;
};

struct AstNode {
	enum NodeKind kind;
	struct AstNode *next; // next node, aka next expr
	struct Token *tok; // the token type of astnode

	struct AstNode *lhs;
	struct AstNode *rhs;

	// if stmt or for stmt
	struct AstNode *condition;
	struct AstNode *then_;
	struct AstNode *else_;
	struct AstNode *init; // init stmt
	struct AstNode *increase; // increase stmt

	// code block
	struct AstNode *body;

	struct Local_Var *var; // string that store var type
	int val;
};

struct Function *parse(struct Token *token);

void codegen(struct Function *prog);
