#include "thrvcc.h"
#include <stdlib.h>

struct Type *TyChar = &(struct Type){ TY_CHAR, 1 };
struct Type *TyInt = &(struct Type){ TY_INT, 8 };

bool is_integer(struct Type *type)
{
	return type->kind == TY_CHAR || type->kind == TY_INT;
}

// copy type
struct Type *copy_type(struct Type *type)
{
	struct Type *ret = calloc(1, sizeof(struct Type));
	*ret = *type;
	return ret;
}

struct Type *pointer_to(struct Type *base)
{
	struct Type *type = calloc(1, sizeof(struct Type));
	type->kind = TY_PTR;
	type->base = base;
	type->size = 8;
	return type;
}

// func type, assign return type
struct Type *func_type(struct Type *return_ty)
{
	struct Type *ty = calloc(1, sizeof(struct Type));
	ty->kind = TY_FUNC;
	ty->return_type = return_ty;
	return ty;
}

// construct array type, pass (array base type, items count)
struct Type *array_of(struct Type *base, int len)
{
	struct Type *type = calloc(1, sizeof(struct Type));
	type->kind = TY_ARRAY;
	// array size = all items size pulse
	type->size = base->size * len;
	type->base = base;
	type->array_len = len;
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
	// Iterate through all nodes of the AST to add types
	for (struct AstNode *nd = node->body; nd; nd = nd->next)
		add_type(nd);
	// Iterate through all parameter nodes to add types
	for (struct AstNode *nd = node->args; nd; nd = nd->next)
		add_type(nd);

	switch (node->kind) {
	case ND_ADD:
	case ND_SUB:
	case ND_MUL:
	case ND_DIV:
	case ND_NEG:
		node->type = node->lhs->type;
		return;
	// set node type as node->lhs type
	// lhs coudn't be array node
	case ND_ASSIGN:
		if (node->lhs->type->kind == TY_ARRAY)
			error_token(node->lhs->tok, "not an lvalue");
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
	case ND_ADDR: {
		struct Type *type = node->lhs->type;
		// if left side is array, its pointer point to array base type
		if (type->kind == TY_ARRAY)
			node->type = pointer_to(type->base);
		else
			node->type = pointer_to(type);
		return;
	}
	case ND_DEREF:
		if (!node->lhs->type->base)
			error_token(node->tok, "invalid pointer dereference");
		node->type = node->lhs->type->base;
		return;
	default:
		break;
	}
}
