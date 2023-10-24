// use POSIX.1 standard
// use func strndup()

#define _POSIX_C_SOURCE 200909L

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

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
	ND_MOD, // %
	ND_BITNOT, // ~
	ND_BITAND, // &
	ND_BITOR, // |
	ND_BITXOR, // ^
	ND_SHL, // <<
	ND_SHR, // >>
	// Relational Operators
	ND_EQ, // ==
	ND_NE, // !=
	ND_LT, // <
	ND_LE, // <=
	// Assignment Operators
	ND_ASSIGN,
	ND_COMMA, // , comma
	ND_MEMBER, // . struct member access
	ND_CAST, // type cast
	ND_MEMZERO, // zeroize variables on the stack
	// Pointer Operators
	ND_ADDR, // get address
	ND_DEREF, // dereference
	// Logical Operators
	ND_NOT, // !
	ND_LOGAND, // &&
	ND_LOGOR, // ||

	// ternary operator
	ND_COND, // ?:

	// keyword
	ND_RETURN, // return
	ND_IF, // if
	ND_FOR, // for or while
	ND_DO, // do, used for do,while stmt
	ND_SWITCH, // switch
	ND_CASE, // case
	ND_BLOCK, // code block
	ND_GOTO, // goto
	ND_LABEL, // goto label
	ND_FUNCALL, // Function call
	ND_VAR, // variable
	ND_NUM,
	ND_EXPR_STMT, // express statement
	ND_STMT_EXPR, // statement express, GNU feature
	ND_NULL_EXPR, // NULL expr
};

enum TypeKind {
	TY_VOID, // void
	TY_BOOL, // _Bool
	TY_CHAR, // char
	TY_SHORT, // short integer
	TY_INT, // integer
	TY_LONG, // long integer
	TY_ENUM, // enum
	TY_PTR, // pointer
	TY_FUNC, // function
	TY_ARRAY, // array
	TY_STRUCT, // struct
	TY_UNION, // union
};

struct Token {
	enum TokenKind kind;
	struct Token *next;
	int64_t val;
	char *location;
	int len;
	struct Type *type;
	char *str;

	int line_no; // line number
};

struct Type {
	enum TypeKind kind; // kind
	int size; // return value of sizeof()
	int align; // alignment

	// pointer
	struct Type *base; // the kind pointed to
	// type name, like variable name, function name.
	struct Token *name; // type name

	// array
	int array_len; // len of array, array items count

	// struct
	struct Member *member;
	bool is_flexible;

	// function type
	struct Type *return_type; // function return type
	struct Type *params; // parameters
	struct Type *next; // next type
};

// struct member
struct Member {
	struct Member *next;
	struct Type *type;
	struct Token *tok; // to improve error-reporting
	struct Token *name;
	int idx;
	int align;
	int offset;
};

struct Relocation {
	struct Relocation *next;
	int offset;
	char *label;
	long addend;
};

// variable or function
struct Obj_Var {
	struct Obj_Var *next; // point to next local_var
	char *name; // local_var name
	struct Type *type; // type
	bool is_local; // is local_var? or global_var?
	int align; // alignment

	// local_var
	int offset; // register fp offset

	// global_var or function
	bool is_function; // is function?
	bool is_definition; // is definition?
	bool is_static; // is it in file scope?

	// global var
	char *init_data; // the data be used to initialize
	struct Relocation *rel; // the pointer point to other global variables

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

	// 'break' label
	char *brk_label;
	// 'continue' label
	char *ctue_label;

	// code block or statement express(GNU)
	struct AstNode *body;

	// struct member access
	struct Member *member; // struct member

	// func call
	char *func_name; // func name
	struct Type *func_type; // func type
	struct AstNode *args; // func args

	// goto and label
	char *label;
	char *unique_label;
	struct AstNode *goto_next;

	// switch and case
	struct AstNode *case_next;
	struct AstNode *default_case;

	struct Obj_Var *var; // string that store var type
	int64_t val;
};

// when error at thrvcc source code, print file_name and line_no
#define unreachable() error_out("internal error at %s:%d", __FILE__, __LINE__)

extern struct Type *TyChar;
extern struct Type *TyInt;
extern struct Type *TyLong;
extern struct Type *TyShort;
extern struct Type *TyVoid;
extern struct Type *TyBool;

void error_out(char *fmt, ...);
void verror_at(int line_no, char *location, char *fmt, va_list va);
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
// enum type
struct Type *enum_type(void);
// struct type
struct Type *struct_type(void);
// function type
struct Type *func_type(struct Type *return_ty);
int align_to(int N, int Align);
// Type conversion, converting the value of an expression to another type
struct AstNode *new_cast(struct AstNode *expr, struct Type *type);
// Lexical analysis
struct Token *lexer_file(char *path);
// Grammatical analysis
struct Obj_Var *parse(struct Token *token);
// Code Generation
void codegen(struct Obj_Var *prog, FILE *out);
