// use POSIX.1 standard
// use func strndup()

#define _POSIX_C_SOURCE 200909L

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

enum TokenKind {
	TK_IDENT, // identifiers
	TK_PUNCT, // operator like "+" "-"
	TK_KEYWORD, // keyword
	TK_STR, // string literals
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
	ND_FUNCALL, // Function call
	ND_VAR, // variable
	ND_NUM,
	ND_EXPR_STMT, // express statement
	ND_STMT_EXPR, // statement express, GNU feature
};

enum TypeKind {
	TY_CHAR, // char
	TY_INT, // integer
	TY_PTR, // pointer
	TY_FUNC, // function
	TY_ARRAY, // array
};

struct Token {
	enum TokenKind kind;
	struct Token *next;
	int val;
	char *location;
	int len;
	struct Type *type;
	char *str;
};

struct Type {
	enum TypeKind kind; // kind
	int size; // return value of sizeof()

	// pointer
	struct Type *base; // the kind pointed to
	// type name, like variable name, function name.
	struct Token *name; // type name

	// array
	int array_len; // len of array, array items count

	// function type
	struct Type *return_type; // function return type
	struct Type *params; // parameters
	struct Type *next; // next type
};

// variable or function
struct Obj_Var {
	struct Obj_Var *next; // point to next local_var
	char *name; // local_var name
	struct Type *type; // type
	bool is_local; // is local_var? or global_var?

	// local_var
	int offset; // register fp offset

	// global_var or function
	bool is_function; // is function?

	// global var
	char *init_data;

	// function
	struct Obj_Var *params; // parameters
	struct AstNode *body; // func body
	struct Obj_Var *locals; // local variables
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

	// code block or statement express(GNU)
	struct AstNode *body;

	// func call
	char *func_name; // func name
	struct AstNode *args; // func args

	struct Obj_Var *var; // string that store var type
	int val;
};

extern struct Type *TyChar;
extern struct Type *TyInt;

void error_out(char *fmt, ...);
void verror_at(char *location, char *fmt, va_list va);
void error_at(char *location, char *fmt, ...);
void error_token(struct Token *token, char *fmt, ...);
// string format
char *format(char *fmt, ...);
// token recognition
bool equal(struct Token *token, char *str);
struct Token *skip(struct Token *token, char *str);
bool consume(struct Token **rest, struct Token *token, char *str);
bool is_integer(struct Type *type);
// copy type
struct Type *copy_type(struct Type *type);
struct Type *pointer_to(struct Type *base);
// add variable kind to all nodes
void add_type(struct AstNode *node);
// array type
struct Type *array_of(struct Type *base, int len);
// function type
struct Type *func_type(struct Type *return_ty);
// Lexical analysis
struct Token *lexer_file(char *path);
// Grammatical analysis
struct Obj_Var *parse(struct Token *token);
// Code Generation
void codegen(struct Obj_Var *prog);
