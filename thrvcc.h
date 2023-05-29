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

enum NodeKind {
	// Arithmetic Operators
	ND_ADD,
	ND_SUB,
	ND_DIV,
	ND_MUL,
	ND_NEG,
	// Relational Operators
	ND_EQ, // ==
	ND_NE, // !=
	ND_LT, // <
	ND_LE, // <=
	// Assignment Operators
	ND_ASSIGN,
	// Pointer Operators
	ND_ADDR, // get address
	ND_DEREF, // dereference

	ND_RETURN, // return
	ND_IF, // if
	ND_FOR, // for or while
	ND_BLOCK, // code block
	ND_VAR, // variable
	ND_NUM,
	ND_EXPR_STMT, // express statement
};

enum TypeKind {
	TY_INT, // integer
	TY_PTR, // pointer
};

struct Token {
	enum TokenKind kind;
	struct Token *next;
	int val;
	char *location;
	int len;
};

struct Type {
	enum TypeKind kind; // kind
	struct Type *base; // the kind pointed to
	struct Token *name; // variable name
};

struct Local_Var {
	struct Local_Var *next; // point to next local_var
	char *name; // local_var name
	struct Type *type; // type
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
	struct Type *type; // the var type in node

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

extern struct Type *TyInt;

void error_out(char *fmt, ...);
void verror_at(char *location, char *fmt, va_list va);
void error_at(char *location, char *fmt, ...);
void error_token(struct Token *token, char *fmt, ...);
// token recognition
bool equal(struct Token *token, char *str);
struct Token *skip(struct Token *token, char *str);
bool consume(struct Token **rest, struct Token *token, char *str);
bool is_integer(struct Type *type);
struct Type *pointer_to(struct Type *base);
// add variable kind to all nodes
void add_type(struct AstNode *node);
// Lexical analysis
struct Token *lexer(char *formula);
// Grammatical analysis
struct Function *parse(struct Token *token);
// Code Generation
void codegen(struct Function *prog);
