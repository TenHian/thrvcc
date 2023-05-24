#include "thrvcc.h"
#include <stdio.h>

static int StackDepth;

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

// calculate the absolute address of the given node
// if error, means node not in menory
static void gen_addr(struct AstNode *node)
{
	if (node->kind == ND_VAR) {
		int offset = (node->name - 'a' + 1) * 8;
		printf("  addi a0, fp, %d\n", -offset);
		return;
	}
	error_out("not an lvalue");
}

static void gen_expr(struct AstNode *node)
{
	switch (node->kind) {
	case ND_NUM:
		printf("  li a0, %d\n", node->val);
		return;
	case ND_NEG:
		gen_expr(node->lhs);
		printf("  neg a0, a0\n");
		return;
	case ND_VAR:
		gen_addr(node);
		printf("  ld a0, 0(a0)\n");
		return;
	case ND_ASSIGN:
		gen_addr(node->lhs);
		push();
		gen_expr(node->rhs);
		pop("a1");
		printf("  sd a0, 0(a1)\n");
		return;
	default:
		break;
	}

	gen_expr(node->rhs);
	push();
	gen_expr(node->lhs);
	pop("a1");

	// gen bin-tree node
	switch (node->kind) {
	case ND_ADD: // + a0=a0+a1
		printf("  add a0, a0, a1\n");
		return;
	case ND_SUB: // - a0=a0-a1
		printf("  sub a0, a0, a1\n");
		return;
	case ND_MUL: // * a0=a0*a1
		printf("  mul a0, a0, a1\n");
		return;
	case ND_DIV: // / a0=a0/a1
		printf("  div a0, a0, a1\n");
		return;
	case ND_EQ:
	case ND_NE:
		// a0=a0^a1
		printf("  xor a0, a0, a1\n");

		if (node->kind == ND_EQ)
			// a0==a1
			// a0=a0^a1, sltiu a0, a0, 1
			// if 0, mk 1
			printf("  seqz a0, a0\n");
		else
			// a0!=a1
			// a0=a0^a1, sltu a0, x0, a0
			// if not eq to 0, turn it into 1
			printf("  snez a0, a0\n");
		return;
	case ND_LT:
		printf("  slt a0, a0, a1\n");
		return;
	case ND_LE:
		// a0<=a1 equal to
		// a0=a1<a0, a0=a1^1
		printf("  slt a0, a1, a0\n");
		printf("  xori a0, a0, 1\n");
		return;
	default:
		break;
	}

	error_out("invalid expression");
}

// gen stmt
static void gen_stmt(struct AstNode *node)
{
	if (node->kind == ND_EXPR_STMT) {
		gen_expr(node->lhs);
		return;
	}

	error_out("invalid statement");
}

void codegen(struct AstNode *node)
{
	printf("  .globl main\n");
	printf("main:\n");

	// stack layout
	//-------------------------------// sp
	//              fp                  fp = sp-8
	//-------------------------------// fp
	//              'a'                 fp-8
	//              'b'                 fp-16
	//              ...
	//              'z'                 fp-208
	//-------------------------------// sp=sp-8-208
	//           expression
	//-------------------------------//

	// prologue
	// push fp, store val of fp
	printf("  addi sp, sp, -8\n");
	printf("  sd fp, 0(sp)\n");
	// write sp into fp
	printf("  mv fp, sp\n");

	// 26 letters, 26*8=208bits, stack alloc 208 bits
	printf("  addi sp, sp, -208\n");

	// Iterate through all statements
	for (struct AstNode *nd = node; nd; nd = nd->next) {
		gen_stmt(nd);
		assert(StackDepth == 0);
	}

	// epilogue
	// write back fp into sp
	printf("  mv sp, fp\n");
	// pop fp, recover fp
	printf("  ld fp, 0(sp)\n");
	printf("  addi sp, sp, 8\n");

	printf("  ret\n");
}
