#include "thrvcc.h"

// program = stmt
// stmt = expr stmt
// expr stmt = expr;
// expr = equality
// equality = relational ("==" | "!=")
// relational = add ("<" | "<=" | ">" | ">=")
// add = mul ("+" | "-")
// mul = unary ("*" | "/")
// unary = ("+" | "-") | primary
// primary = "(" expr ")" | num

static struct AstNode *stmt(struct Token **rest, struct Token *token);
static struct AstNode *exprstmt(struct Token **rest, struct Token *token);
static struct AstNode *equality(struct Token **rest, struct Token *token);
static struct AstNode *relational(struct Token **rest, struct Token *token);
static struct AstNode *expr(struct Token **rest, struct Token *token);
static struct AstNode *add(struct Token **rest, struct Token *token);
static struct AstNode *mul(struct Token **rest, struct Token *token);
static struct AstNode *unary(struct Token **rest, struct Token *token);
static struct AstNode *primary(struct Token **rest, struct Token *token);

static struct AstNode *new_astnode(enum NodeKind kind)
{
	struct AstNode *node = calloc(1, sizeof(struct AstNode));
	node->kind = kind;
	return node;
}

static struct AstNode *new_unary_tree_node(enum NodeKind kind,
					   struct AstNode *expr)
{
	struct AstNode *node = new_astnode(kind);
	node->lhs = expr;
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

// parse stmt
// stmt = exprStmt
static struct AstNode *stmt(struct Token **rest, struct Token *token)
{
	return exprstmt(rest, token);
}

// parse expr
// exprStmt = expr ;
static struct AstNode *exprstmt(struct Token **rest, struct Token *token)
{
	struct AstNode *node = new_unary_tree_node(ND_EXPR_STMT, expr(&token, token));
	*rest = skip(token, ";");
	return node;
}

// expr = equality
static struct AstNode *expr(struct Token **rest, struct Token *token)
{
	return equality(rest, token);
}

// equality = relational ("==" relational | "!=" relational)*
static struct AstNode *equality(struct Token **rest, struct Token *token)
{
	// relational
	struct AstNode *node = relational(&token, token);

	// ("==" relational | "!=" relational)*
	while (true) {
		// "==" relational
		if (equal(token, "==")) {
			node = new_binary_tree_node(
				ND_EQ, node, relational(&token, token->next));
			continue;
		}
		// "!=" relational
		if (equal(token, "!=")) {
			node = new_binary_tree_node(
				ND_NE, node, relational(&token, token->next));
			continue;
		}

		*rest = token;
		return node;
	}
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static struct AstNode *relational(struct Token **rest, struct Token *token)
{
	// add
	struct AstNode *node = add(&token, token);

	// ("<" add | "<=" add | ">" add | ">=" add)*
	while (true) {
		// "<" add
		if (equal(token, "<")) {
			node = new_binary_tree_node(ND_LT, node,
						    add(&token, token->next));
			continue;
		}
		// "<=" add
		if (equal(token, "<=")) {
			node = new_binary_tree_node(ND_LE, node,
						    add(&token, token->next));
			continue;
		}
		// ">" add, X>Y equal Y<X
		if (equal(token, ">")) {
			node = new_binary_tree_node(
				ND_LT, add(&token, token->next), node);
			continue;
		}
		// ">=" add
		if (equal(token, ">=")) {
			node = new_binary_tree_node(
				ND_LE, add(&token, token->next), node);
			continue;
		}

		*rest = token;
		return node;
	}
}

// expr = mul ("+" mul | "-" mul)*
static struct AstNode *add(struct Token **rest, struct Token *token)
{
	struct AstNode *node = mul(&token, token);

	while (true) {
		if (equal(token, "+")) {
			node = new_binary_tree_node(ND_ADD, node,
						    mul(&token, token->next));
			continue;
		}
		if (equal(token, "-")) {
			node = new_binary_tree_node(ND_SUB, node,
						    mul(&token, token->next));
			continue;
		}
		*rest = token;
		return node;
	}
}
// mul = primary ("*" primary | "/" primary) *
static struct AstNode *mul(struct Token **rest, struct Token *token)
{
	// unary
	struct AstNode *node = unary(&token, token);

	while (true) {
		if (equal(token, "*")) {
			node = new_binary_tree_node(ND_MUL, node,
						    unary(&token, token->next));
			continue;
		}
		if (equal(token, "/")) {
			node = new_binary_tree_node(ND_DIV, node,
						    unary(&token, token->next));
			continue;
		}

		*rest = token;
		return node;
	}
}

static struct AstNode *unary(struct Token **rest, struct Token *token)
{
	if (equal(token, "+"))
		return unary(rest, token->next);
	if (equal(token, "-"))
		return new_unary_tree_node(ND_NEG, unary(rest, token->next));

	return primary(rest, token);
}

// parse "(" ")" and num
static struct AstNode *primary(struct Token **rest, struct Token *token)
{
	// "(" expr ")"
	if (equal(token, "(")) {
		struct AstNode *node = expr(&token, token->next);
		*rest = skip(token, ")");
		return node;
	}
	// num
	if (token->kind == TK_NUM) {
		struct AstNode *node = new_num_astnode(token->val);
		*rest = token->next;
		return node;
	}

	error_token(token, "expected an expression");
	return NULL;
}

struct AstNode *parse(struct Token *token)
{
	struct AstNode head = {};
	struct AstNode *cur = &head;
	// stmt*
	while(token->kind!=TK_EOF){
		cur->next = stmt(&token, token);
		cur = cur->next;
	}
	return head.next;
}