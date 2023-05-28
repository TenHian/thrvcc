#include "thrvcc.h"
#include <assert.h>
#include <time.h>

// all var add in this list while parse
struct Local_Var *locals;

// program = "{" compoundStmt
// compoundStmt = stmt* "}"
// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" exprStmt expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "{" compoundStmt
//      | exprStmt
// expr stmt = expr? ;
// expr = assign
// assign = equality ("=" assign)?
// equality = relational ("==" | "!=")
// relational = add ("<" | "<=" | ">" | ">=")
// add = mul ("+" | "-")
// mul = unary ("*" | "/")
// unary = ("+" | "-" | "*" | "&") unary | primary
// primary = "(" expr ")" | ident |num

static struct AstNode *compoundstmt(struct Token **rest, struct Token *token);
static struct AstNode *stmt(struct Token **rest, struct Token *token);
static struct AstNode *exprstmt(struct Token **rest, struct Token *token);
static struct AstNode *assign(struct Token **rest, struct Token *token);
static struct AstNode *equality(struct Token **rest, struct Token *token);
static struct AstNode *relational(struct Token **rest, struct Token *token);
static struct AstNode *expr(struct Token **rest, struct Token *token);
static struct AstNode *add(struct Token **rest, struct Token *token);
static struct AstNode *mul(struct Token **rest, struct Token *token);
static struct AstNode *unary(struct Token **rest, struct Token *token);
static struct AstNode *primary(struct Token **rest, struct Token *token);

// find a local var by search name
static struct Local_Var *find_var(struct Token *token)
{
	for (struct Local_Var *var = locals; var; var = var->next)
		if (strlen(var->name) == token->len &&
		    !strncmp(token->location, var->name, token->len))
			return var;
	return NULL;
}

static struct AstNode *new_astnode(enum NodeKind kind, struct Token *token)
{
	struct AstNode *node = calloc(1, sizeof(struct AstNode));
	node->kind = kind;
	node->tok = token;
	return node;
}

static struct AstNode *new_unary_tree_node(enum NodeKind kind,
					   struct AstNode *expr,
					   struct Token *token)
{
	struct AstNode *node = new_astnode(kind, token);
	node->lhs = expr;
	return node;
}

static struct AstNode *new_binary_tree_node(enum NodeKind kind,
					    struct AstNode *lhs,
					    struct AstNode *rhs,
					    struct Token *token)
{
	struct AstNode *node = new_astnode(kind, token);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

static struct AstNode *new_num_astnode(int val, struct Token *token)
{
	struct AstNode *node = new_astnode(ND_NUM, token);
	node->val = val;
	return node;
}

// add a var into global var list
static struct Local_Var *new_lvar(char *name)
{
	struct Local_Var *var = calloc(1, sizeof(struct Local_Var));
	var->name = name;
	// insert into head
	var->next = locals;
	locals = var;
	return var;
}

static struct AstNode *new_var_astnode(struct Local_Var *var,
				       struct Token *token)
{
	struct AstNode *node = new_astnode(ND_VAR, token);
	node->var = var;
	return node;
}

// parse stmt
// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" exprStmt expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "{" compoundStmt
//      | exprStmt
static struct AstNode *stmt(struct Token **rest, struct Token *token)
{
	// "return" expr ";"
	if (equal(token, "return")) {
		struct AstNode *node = new_astnode(ND_RETURN, token);
		node->lhs = expr(&token, token->next);
		*rest = skip(token, ";");
		return node;
	}
	// "if" "(" expr ")" stmt ("else" stmt)?
	if (equal(token, "if")) {
		struct AstNode *node = new_astnode(ND_IF, token);
		// conditionial stmt
		token = skip(token->next, "(");
		node->condition = expr(&token, token);
		token = skip(token, ")");
		// then stmt
		node->then_ = stmt(&token, token);
		// ("else" stmt)?
		if (equal(token, "else"))
			node->else_ = stmt(&token, token->next);
		*rest = token;
		return node;
	}
	// "for" "(" exprStmt expr? ";" expr? ")" stmt
	if (equal(token, "for")) {
		struct AstNode *node = new_astnode(ND_FOR, token);
		// "("
		token = skip(token->next, "(");

		// exprStmt
		node->init = exprstmt(&token, token);

		// expr?
		if (!equal(token, ";"))
			node->condition = expr(&token, token);
		// ";"
		token = skip(token, ";");

		// expr?
		if (!equal(token, ")"))
			node->increase = expr(&token, token);
		// ")"
		token = skip(token, ")");

		// stmt
		node->then_ = stmt(rest, token);
		return node;
	}
	// "while" "(" expr ")" stmt
	if (equal(token, "while")) {
		struct AstNode *node = new_astnode(ND_FOR, token);
		// "("
		token = skip(token->next, "(");
		// expr
		node->condition = expr(&token, token);
		// ")"
		token = skip(token, ")");
		// stmt
		node->then_ = stmt(rest, token);
		return node;
	}

	// "{" compoundStmt
	if (equal(token, "{"))
		return compoundstmt(rest, token->next);
	// exprStmt
	return exprstmt(rest, token);
}

// parse compound stmt
// compoundStmt = stmt* "}"
static struct AstNode *compoundstmt(struct Token **rest, struct Token *token)
{
	struct AstNode *node = new_astnode(ND_BLOCK, token);
	// unidirectional linked list
	struct AstNode head = {};
	struct AstNode *cur = &head;
	// stmt* "}"
	while (!equal(token, "}")) {
		cur->next = stmt(&token, token);
		cur = cur->next;
		// add var kind to nodes, after constructed AST
		add_type(cur);
	}
	// mov stmt that in "{}" to parser
	node->body = head.next;
	*rest = token->next;
	return node;
}

// parse expr
// exprStmt = expr ;
static struct AstNode *exprstmt(struct Token **rest, struct Token *token)
{
	// ";"
	if (equal(token, ";")) {
		*rest = token->next;
		return new_astnode(ND_BLOCK, token);
	}
	// expr ";"
	struct AstNode *node = new_astnode(ND_EXPR_STMT, token);
	node->lhs = expr(&token, token);
	*rest = skip(token, ";");
	return node;
}

// expr = assign
static struct AstNode *expr(struct Token **rest, struct Token *token)
{
	return assign(rest, token);
}

// assign = equality
static struct AstNode *assign(struct Token **rest, struct Token *token)
{
	// equality
	struct AstNode *node = equality(&token, token);
	// there may be recursive assignments, such as a=b=1
	// ("=" assign)?
	if (equal(token, "="))
		return node = new_binary_tree_node(ND_ASSIGN, node,
						   assign(rest, token->next),
						   token);
	*rest = token;
	return node;
}

// equality = relational ("==" relational | "!=" relational)*
static struct AstNode *equality(struct Token **rest, struct Token *token)
{
	// relational
	struct AstNode *node = relational(&token, token);

	// ("==" relational | "!=" relational)*
	while (true) {
		struct Token *start = token;
		// "==" relational
		if (equal(token, "==")) {
			node = new_binary_tree_node(
				ND_EQ, node, relational(&token, token->next),
				start);
			continue;
		}
		// "!=" relational
		if (equal(token, "!=")) {
			node = new_binary_tree_node(
				ND_NE, node, relational(&token, token->next),
				start);
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
		struct Token *start = token;
		// "<" add
		if (equal(token, "<")) {
			node = new_binary_tree_node(
				ND_LT, node, add(&token, token->next), start);
			continue;
		}
		// "<=" add
		if (equal(token, "<=")) {
			node = new_binary_tree_node(
				ND_LE, node, add(&token, token->next), start);
			continue;
		}
		// ">" add, X>Y equal Y<X
		if (equal(token, ">")) {
			node = new_binary_tree_node(
				ND_LT, add(&token, token->next), node, start);
			continue;
		}
		// ">=" add
		if (equal(token, ">=")) {
			node = new_binary_tree_node(
				ND_LE, add(&token, token->next), node, start);
			continue;
		}

		*rest = token;
		return node;
	}
}

// parse various add
static struct AstNode *new_add(struct AstNode *lhs, struct AstNode *rhs,
			       struct Token *token)
{
	// add type for both left side and right side
	add_type(lhs);
	add_type(rhs);
	// num + num
	if (is_integer(lhs->type) && is_integer(rhs->type))
		return new_binary_tree_node(ND_ADD, lhs, rhs, token);
	// if "ptr + ptr", error
	if (lhs->type->base && rhs->type->base)
		error_token(token, "invalid operands");
	// turn "num + ptr" into "ptr + num"
	if (!lhs->type->base && rhs->type->base) {
		struct AstNode *tmp = lhs;
		lhs = rhs;
		rhs = tmp;
	}
	// ptr + num
	// pointer additon, ptr+1, the 1 in here is not a char,
	// is 1 item's space, so, need to x8
	rhs = new_binary_tree_node(ND_MUL, rhs, new_num_astnode(8, token),
				   token);
	return new_binary_tree_node(ND_ADD, lhs, rhs, token);
}

// parse various sub
static struct AstNode *new_sub(struct AstNode *lhs, struct AstNode *rhs,
			       struct Token *token)
{
	add_type(lhs);
	add_type(rhs);

	// num - num
	if (is_integer(lhs->type) && is_integer(rhs->type))
		return new_binary_tree_node(ND_SUB, lhs, rhs, token);
	// ptr - num
	if (lhs->type->base && is_integer(rhs->type)) {
		rhs = new_binary_tree_node(ND_MUL, rhs,
					   new_num_astnode(8, token), token);
		add_type(rhs);
		struct AstNode *node =
			new_binary_tree_node(ND_SUB, lhs, rhs, token);
		// node type is pointer
		node->type = lhs->type;
		return node;
	}
	// ptr - ptr, return how many items between 2 pointers
	if (lhs->type->base && rhs->type->base) {
		struct AstNode *node =
			new_binary_tree_node(ND_SUB, lhs, rhs, token);
		node->type = TyInt;
		return new_binary_tree_node(ND_DIV, node,
					    new_num_astnode(8, token), token);
	}
	error_token(token, "invalid operands");
	return NULL;
}

// expr = mul ("+" mul | "-" mul)*
static struct AstNode *add(struct Token **rest, struct Token *token)
{
	struct AstNode *node = mul(&token, token);

	while (true) {
		struct Token *start = token;
		if (equal(token, "+")) {
			node = new_add(node, mul(&token, token->next), start);
			continue;
		}
		if (equal(token, "-")) {
			node = new_sub(node, mul(&token, token->next), start);
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
		struct Token *start = token;
		if (equal(token, "*")) {
			node = new_binary_tree_node(ND_MUL, node,
						    unary(&token, token->next),
						    start);
			continue;
		}
		if (equal(token, "/")) {
			node = new_binary_tree_node(ND_DIV, node,
						    unary(&token, token->next),
						    start);
			continue;
		}

		*rest = token;
		return node;
	}
}

// parse unary operators
// unary = ("+" | "-" | "*" | "&") unary | primary
static struct AstNode *unary(struct Token **rest, struct Token *token)
{
	if (equal(token, "+"))
		return unary(rest, token->next);
	if (equal(token, "-"))
		return new_unary_tree_node(ND_NEG, unary(rest, token->next),
					   token);
	if (equal(token, "&"))
		return new_unary_tree_node(ND_ADDR, unary(rest, token->next),
					   token);
	if (equal(token, "*"))
		return new_unary_tree_node(ND_DEREF, unary(rest, token->next),
					   token);

	return primary(rest, token);
}

// parse "(" ")" | num | variables
static struct AstNode *primary(struct Token **rest, struct Token *token)
{
	// "(" expr ")"
	if (equal(token, "(")) {
		struct AstNode *node = expr(&token, token->next);
		*rest = skip(token, ")");
		return node;
	}
	// ident
	if (token->kind == TK_IDENT) {
		// find var
		struct Local_Var *var = find_var(token);
		// if var not exist, creat a new var in global var list
		if (!var)
			var = new_lvar(strndup(token->location, token->len));
		*rest = token->next;
		return new_var_astnode(var, token);
	}
	// num
	if (token->kind == TK_NUM) {
		struct AstNode *node = new_num_astnode(token->val, token);
		*rest = token->next;
		return node;
	}

	error_token(token, "expected an expression");
	return NULL;
}

// parser
// program = "{" compoundStmt
struct Function *parse(struct Token *token)
{
	// "{"
	token = skip(token, "{");

	struct Function *prog = calloc(1, sizeof(struct Function));
	prog->body = compoundstmt(&token, token);
	prog->locals = locals;
	return prog;
}
