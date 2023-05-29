#include "thrvcc.h"

struct Type *TyInt = &(struct Type){ TY_INT };

bool is_integer(struct Type *type)
{
	return type->kind == TY_INT;
}

struct Type *pointer_to(struct Type *base)
{
	struct Type *type = calloc(1, sizeof(struct Type));
	type->kind = TY_PTR;
	type->base = base;
	return type;
}

// add type for nodes
void add_type(struct AstNode *node)
{
	// judge, if node is empty or node haa a kind, return
	if (!node || node->type)
		return;
	// Recursively access all nodes to add types
	add_type(node->lhs);
	add_type(node->rhs);
	add_type(node->condition);
	add_type(node->then_);
	add_type(node->else_);
	add_type(node->init);
	add_type(node->increase);
	for (struct AstNode *nd = node->body; nd; nd = nd->next)
		add_type(nd);

	switch (node->kind) {
	case ND_ADD:
	case ND_SUB:
	case ND_MUL:
	case ND_DIV:
	case ND_NEG:
	case ND_ASSIGN:
		node->type = node->lhs->type;
		return;
	case ND_EQ:
	case ND_NE:
	case ND_LT:
	case ND_LE:
	case ND_NUM:
	case ND_FUNCALL:
		node->type = TyInt;
		return;
	case ND_VAR:
		node->type = node->var->type;
		return;
	case ND_ADDR:
		node->type = pointer_to(node->lhs->type);
		return;
	case ND_DEREF:
		if (node->lhs->type->kind != TY_PTR)
			error_token(node->tok, "invalid pointer dereference");
		node->type = node->lhs->type->base;
		return;
	default:
		break;
	}
}
