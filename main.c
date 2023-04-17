#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static char *InputArgv;		// reg the argv[1]
static int StackDepth;

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

static void verror_at(char *location, char *fmt, va_list va)
{
	fprintf(stderr, "%s\n", InputArgv);

	int pos = location - InputArgv;
	fprintf(stderr, "%*s", pos, " ");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
}

static void error_at(char *location, char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	verror_at(location, fmt, va);
	exit(1);
}

static void error_token(struct Token *token, char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	verror_at(token->location, fmt, va);
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
		error_token(token, "expect '%s'", str);
	return token->next;
}

static int get_number(struct Token *token)
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

// Terminator Parser
static struct Token *token_parser()
{
	char *formula = InputArgv;
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
		if (ispunct(*formula)) {
			cur->next = new_token(TK_PUNCT, formula, formula + 1);
			cur = cur->next;
			++formula;
			continue;
		}

		error_at(formula, "invalid token");
	}

	cur->next = new_token(TK_EOF, formula, formula);
	return head.next;
}

/*
 * generate AST, Implementing a syntax parser
*/

// def AST node's kind
enum NodeKind {
	ND_ADD,
	ND_SUB,
	ND_DIV,
	ND_MUL,
	ND_NUM,
};

struct AstNode {
	enum NodeKind kind;
	struct AstNode *lhs;
	struct AstNode *rhs;
	int val;
};

static struct AstNode *new_astnode(enum NodeKind kind)
{
	struct AstNode *node = calloc(1, sizeof(struct AstNode));
	node->kind = kind;
	return node;
}

static struct AstNode *new_binary_tree_node(enum NodeKind kind,
					    struct AstNode *lhs,
					    struct AstNode *rhs)
{
	struct AstNode *node = new_astnode(kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

static struct AstNode *new_num_astnode(int val)
{
	struct AstNode *node = new_astnode(ND_NUM);
	node->val = val;
	return node;
}

static struct AstNode *expr(struct Token **rest, struct Token *token);
static struct AstNode *mul(struct Token **rest, struct Token *token);
static struct AstNode *primary(struct Token **rest, struct Token *token);

// expr = mul ("+" mul | "-" mul)*
static struct AstNode *expr(struct Token **rest, struct Token *token)
{
	struct AstNode *node = mul(&token, token);

	while (true) {
		if (equal(token, "+")) {
			node = new_binary_tree_node(ND_ADD, node,
						    mul(&token, token->next));
			continue;
		}
		if(equal(token, "-")) {
			node = new_binary_tree_node(ND_SUB, node,
						    mul(&token, token->next));
			continue ;
		}
		*rest = token;
		return node;
	}
}
// mul = primary ("*" primary | "/" primary) *
static struct AstNode *mul(struct Token **rest, struct Token *token)
{
	struct AstNode *node = primary(&token, token);

	while (true) {
		if(equal(token, "*")) {
			node = new_binary_tree_node(ND_MUL, node, primary(&token, token->next));
			continue ;
		}
		if(equal(token, "/")) {
			node = new_binary_tree_node(ND_DIV, node, primary(&token, token->next));
			continue ;
		}

		*rest = token;
		return node;
	}
}

// parse "(" ")" and num
static struct AstNode *primary(struct Token **rest, struct Token *token)
{
	// "(" expr ")"
	if(equal(token, "(")) {
		struct AstNode *node = expr(&token, token->next);
		*rest = skip(token, ")");
		return node;
	}
	// num
	if(token->kind == TK_NUM) {
		struct AstNode *node = new_num_astnode(token->val);
		*rest = token->next;
		return node;
	}

	error_token(token, "expected an expression");
	return NULL;
}

// push stack, mov the tem result into stack
// *sp* is stack pointer, stack grows down, 8 bytes to a unit at 64 bits mode
// *sp* point to cur stack, so, push a0 into stack
// the reason for not using register storage is because \
// the number of values to be stored is variable
static void push(void)
{
	printf("  addi sp, sp, -8\n");
	printf("  sd a0, 0(sp)\n");
	StackDepth++;
}

// pop stack, mov the top val of stack into a1
static void pop(char *reg)
{
	printf("  ld %s, 0(sp)\n", reg);
	printf("  addi sp, sp, 8\n");
	StackDepth--;
}

static void gen_expr(struct AstNode *node)
{
	if(node->kind == ND_NUM) {
		printf("  li a0, %d\n", node->val);
		return ;
	}

	gen_expr(node->rhs);
	push();
	gen_expr(node->lhs);
	pop("a1");

	// gen bin-tree node
	switch (node->kind) {
	case ND_ADD: // + a0=a0+a1
		printf("  add a0, a0, a1\n");
		return ;
	case ND_SUB: // - a0=a0-a1
		printf("  sub a0, a0, a1\n");
		return ;
	case ND_MUL: // * a0=a0*a1
		printf("  mul a0, a0, a1\n");
		return ;
	case ND_DIV: // / a0=a0/a1
		printf("  div a0, a0, a1\n");
		return ;
	default:
		break ;
	}

	error_out("invalid expression");
}


int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
		return 1;
	}

	// parse argv[1], gen token stream
	InputArgv = argv[1];
	struct Token *token = token_parser(argv[1]);

	// parse token stream
	struct AstNode *ast_node = expr(&token, token);
	if(token->kind != TK_EOF)
		error_token(token, "extra token");

	printf("  .globl main\n");
	printf("main:\n");
	// iterate through the AST tree to generate assembly code
	gen_expr(ast_node);
	printf("  ret\n");

	// if stack is not empty, error
	assert(StackDepth == 0);

	return 0;
}