#include "thrvcc.h"

struct Type *TyVoid = &(struct Type){ TY_VOID, 1, 1 };
struct Type *TyBool = &(struct Type){ TY_BOOL, 1, 1 };
struct Type *TyChar = &(struct Type){ TY_CHAR, 1, 1 };
struct Type *TyUChar = &(struct Type){ TY_CHAR, 1, 1, true };
struct Type *TyShort = &(struct Type){ TY_SHORT, 2, 2 };
struct Type *TyUShort = &(struct Type){ TY_SHORT, 2, 2, true };
struct Type *TyInt = &(struct Type){ TY_INT, 4, 4 };
struct Type *TyUInt = &(struct Type){ TY_INT, 4, 4, true };
struct Type *TyLong = &(struct Type){ TY_LONG, 8, 8 };
struct Type *TyULong = &(struct Type){ TY_LONG, 8, 8, true };
struct Type *TyFloat = &(struct Type){ TY_FLOAT, 4, 4 };
struct Type *TyDouble = &(struct Type){ TY_DOUBLE, 8, 8 };

static struct Type *new_type(enum TypeKind ty_kind, int size, int align)
{
	struct Type *type = calloc(1, sizeof(struct Type));
	type->kind = ty_kind;
	type->size = size;
	type->align = align;
	return type;
}

bool is_integer(struct Type *type)
{
	return type->kind == TY_BOOL || type->kind == TY_CHAR ||
	       type->kind == TY_SHORT || type->kind == TY_INT ||
	       type->kind == TY_LONG || type->kind == TY_ENUM;
}

bool is_float(struct Type *type)
{
	return type->kind == TY_FLOAT || type->kind == TY_DOUBLE;
}

bool is_numeric(struct Type *type)
{
	return is_integer(type) || is_float(type);
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
	struct Type *type = new_type(TY_PTR, 8, 8);
	type->base = base;
	// regard pointers as unsigned types
	type->is_unsigned = true;
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
	struct Type *type = new_type(TY_ARRAY, base->size * len, base->align);
	type->base = base;
	type->array_len = len;
	return type;
}

// construct enum type
struct Type *enum_type(void)
{
	return new_type(TY_ENUM, 4, 4);
}

// construct struct type
struct Type *struct_type(void)
{
	return new_type(TY_STRUCT, 0, 1);
}

// Get the type of accommodating left and right parts
static struct Type *get_common_type(struct Type *ty1, struct Type *ty2)
{
	if (ty1->base)
		return pointer_to(ty1->base);
	// handle float type
	// handle double first
	if (ty1->kind == TY_DOUBLE || ty2->kind == TY_DOUBLE)
		return TyDouble;
	// then handle float
	if (ty1->kind == TY_FLOAT || ty2->kind == TY_FLOAT)
		return TyFloat;

	// if less than 4 bytes, its integer
	if (ty1->size < 4)
		ty1 = TyInt;
	if (ty2->size < 4)
		ty2 = TyInt;
	// choose bigger one
	if (ty1->size != ty2->size)
		return (ty1->size < ty2->size) ? ty2 : ty1;
	// priority return unsigned type(bigger)
	if (ty2->is_unsigned)
		return ty2;
	return ty1;
}

// Perform regular arithmetic conversions
static void arith_conv(struct AstNode **lhs, struct AstNode **rhs)
{
	struct Type *type = get_common_type((*lhs)->type, (*rhs)->type);
	// Converting left and right parts to compatible types
	*lhs = new_cast(*lhs, type);
	*rhs = new_cast(*rhs, type);
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
	// set node type as int
	case ND_NUM:
		node->type = TyInt;
		return;
	// set node type as left node's type
	case ND_ADD:
	case ND_SUB:
	case ND_MUL:
	case ND_DIV:
	case ND_MOD:
	case ND_BITAND:
	case ND_BITOR:
	case ND_BITXOR:
		// type conversion for left and right parts
		arith_conv(&node->lhs, &node->rhs);
		node->type = node->lhs->type;
		return;
	case ND_NEG: {
		// type conversion for left part
		struct Type *type = get_common_type(TyInt, node->lhs->type);
		node->lhs = new_cast(node->lhs, type);
		node->type = type;
		return;
	}
	// set node type as node->lhs type
	// lhs coudn't be array node
	case ND_ASSIGN:
		if (node->lhs->type->kind == TY_ARRAY)
			error_token(node->lhs->tok, "not an lvalue");
		if (node->lhs->type->kind != TY_STRUCT)
			// type conversion for right part
			node->rhs = new_cast(node->rhs, node->lhs->type);
		node->type = node->lhs->type;
		return;
	case ND_EQ:
	case ND_NE:
	case ND_LT:
	case ND_LE:
		// type conversion for left and right part
		arith_conv(&node->lhs, &node->rhs);
		node->type = TyInt;
		return;
	case ND_FUNCALL:
		node->type = node->func_type->return_type;
		return;
	case ND_NOT:
	case ND_LOGAND:
	case ND_LOGOR:
		node->type = TyInt;
		return;
	case ND_BITNOT:
	case ND_SHL:
	case ND_SHR:
		node->type = node->lhs->type;
		return;
	case ND_VAR:
		node->type = node->var->type;
		return;
	case ND_COND:
		if (node->then_->type->kind == TY_VOID ||
		    node->else_->type->kind == TY_VOID)
			node->type = TyVoid;
		else {
			arith_conv(&node->then_, &node->else_);
			node->type = node->then_->type;
		}
		return;
	// make astnode type as rhs type
	case ND_COMMA:
		node->type = node->rhs->type;
		return;
	case ND_MEMBER:
		node->type = node->member->type;
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
		if (node->lhs->type->base->kind == TY_VOID)
			error_token(node->tok, "dereferencing a void pointer");
		node->type = node->lhs->type->base;
		return;
	case ND_STMT_EXPR:
		if (node->body) {
			struct AstNode *stmt = node->body;
			while (stmt->next)
				stmt = stmt->next;
			if (stmt->kind == ND_EXPR_STMT) {
				node->type = stmt->lhs->type;
				return;
			}
		}
		error_token(
			node->tok,
			"statement expression returning void is not supported");
		return;
	default:
		break;
	}
}
