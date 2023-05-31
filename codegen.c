#include "thrvcc.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// used to record stack depth
// used by func:
//         static void push(void)
//         static void pop(char *reg)
//         void codegen(struct Function *prog)
static int StackDepth;

// the registers that used to store func args
// used by func
//         static void gen_expr(struct AstNode *node)
static char *ArgsReg[] = { "a0", "a1", "a2", "a3", "a4", "a5" };
// current func
static struct Function *CurFn;

static void gen_expr(struct AstNode *node);

// code block count
static int count(void)
{
	static int Count_Num = 1;
	return Count_Num++;
}

// push stack, mov the tem result into stack
// *sp* is stack pointer, stack grows down, 8 bytes to a unit at 64 bits mode
// *sp* point to cur stack, so, push a0 into stack
// the reason for not using register storage is because \
// the number of values to be stored is variable
static void push(void)
{
	printf("  # push a0 into the top of the stack\n");
	printf("  addi sp, sp, -8\n");
	printf("  sd a0, 0(sp)\n");
	StackDepth++;
}

// pop stack, mov the top val of stack into a1
static void pop(char *reg)
{
	printf("  # pop out the top of the stack, and mov it into %s\n", reg);
	printf("  ld %s, 0(sp)\n", reg);
	printf("  addi sp, sp, 8\n");
	StackDepth--;
}

// align to an integer multiple of align
static int align_to(int N, int align)
{
	// (0, align] return align
	return (N + align + 1) / align * align;
}

// calculate the absolute address of the given node
// if error, means node not in menory
static void gen_addr(struct AstNode *node)
{
	switch (node->kind) {
	case ND_VAR:
		// offset fp
		printf("  # get varable %s's address in stack as %d(fp)\n",
		       node->var->name, node->var->offset);
		printf("  addi a0, fp, %d\n", node->var->offset);
		return;
	case ND_DEREF:
		gen_expr(node->lhs);
		return;
	default:
		break;
	}
	error_token(node->tok, "not an lvalue");
}

static void gen_expr(struct AstNode *node)
{
	switch (node->kind) {
	case ND_NUM:
		printf("  # load %d into a0\n", node->val);
		printf("  li a0, %d\n", node->val);
		return;
	case ND_NEG:
		gen_expr(node->lhs);
		printf("  # reverse the value of a0\n");
		printf("  neg a0, a0\n");
		return;
	case ND_VAR:
		gen_addr(node);
		printf("  # read the address that stored in a0, then load it's value into a0\n");
		printf("  ld a0, 0(a0)\n");
		return;
	case ND_DEREF:
		gen_expr(node->lhs);
		printf("  # read the address that stored in a0, store its val in a0\n");
		printf("  ld a0, 0(a0)\n");
		return;
	case ND_ADDR:
		gen_addr(node->lhs);
		return;
	case ND_ASSIGN:
		gen_addr(node->lhs);
		push();
		gen_expr(node->rhs);
		pop("a1");
		printf("  # wirte the value of a0 into the address that stored by a1\n");
		printf("  sd a0, 0(a1)\n");
		return;
	case ND_FUNCALL: {
		// args count
		int args_count = 0;
		// cau all args' value, push stack forward
		for (struct AstNode *arg = node->args; arg; arg = arg->next) {
			gen_expr(arg);
			push();
			args_count++;
		}
		// pop stack, a0->arg1, a1->arg2
		for (int i = args_count - 1; i >= 0; i--)
			pop(ArgsReg[i]);

		// func call
		printf("  # func call %s\n", node->func_name);
		printf("  call %s\n", node->func_name);
		return;
	}
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
		printf("  # a0+a1, write the result into a0\n");
		printf("  add a0, a0, a1\n");
		return;
	case ND_SUB: // - a0=a0-a1
		printf("  # a0-a1, write the result into a0\n");
		printf("  sub a0, a0, a1\n");
		return;
	case ND_MUL: // * a0=a0*a1
		printf("  # a0xa1, write the result into a0\n");
		printf("  mul a0, a0, a1\n");
		return;
	case ND_DIV: // / a0=a0/a1
		printf("  # a0/a1, write the result into a0\n");
		printf("  div a0, a0, a1\n");
		return;
	case ND_EQ:
	case ND_NE:
		// a0=a0^a1
		printf("  # determine a0%sa1\n",
		       node->kind == ND_EQ ? "=" : "!=");
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
		printf("  # determine a0<a1?\n");
		printf("  slt a0, a0, a1\n");
		return;
	case ND_LE:
		// a0<=a1 equal to
		// a0=a1<a0, a0=a1^1
		printf("  #determine a0<=a1\n");
		printf("  slt a0, a1, a0\n");
		printf("  xori a0, a0, 1\n");
		return;
	default:
		break;
	}

	error_token(node->tok, "invalid expression");
}

// gen stmt
static void gen_stmt(struct AstNode *node)
{
	switch (node->kind) {
	// if stmt
	case ND_IF: {
		// code block count
		int C = count();
		printf("\n# =====Branch Statement %d =====\n", C);
		// gen conditional stmt
		printf("\n# Conditional Statement %d\n", C);
		gen_expr(node->condition);
		// determine condition, if 0 jump to label else
		printf("  # if a0 == 0, jump to branch%d's .L.else.%d segment\n",
		       C, C);
		printf("  beqz a0, .L.else.%d\n", C);
		// then stmt
		printf("\n# then statement%d\n", C);
		gen_stmt(node->then_);
		// over, jump to stmt after if stmt
		printf("  # jump to branch%d's .L.end.%d segment\n", C, C);
		printf("  j .L.end.%d\n", C);
		// else block, else block may empty, so output its label
		printf("\n# else statement%d\n", C);
		printf("# branch%d's .L.else.%d segment label\n", C, C);
		printf(".L.else.%d:\n", C);
		// gen else_
		if (node->else_)
			gen_stmt(node->else_);
		printf("\n# branch%d's .L.end.%d segment label\n", C, C);
		printf(".L.end.%d:\n", C);
		return;
	}
	// for stmt or while stmt
	case ND_FOR: {
		// code block count
		int C = count();
		printf("\n# =====Loop Statement %d =====\n", C);
		// gen init stmt
		if (node->init) {
			printf("\n# Init Statement%d\n", C);
			gen_stmt(node->init);
		}
		// output loop's head label
		printf("\n# loop%d's .L.begin.%d segment label\n", C, C);
		printf(".L.begin.%d:\n", C);
		// process loop's conditional stmt
		printf("\n# Conditional Statement %d\n", C);
		if (node->condition) {
			// gen loop's conditional stmt
			gen_expr(node->condition);
			// determine condition, if 0 jump to loop's end
			printf("  # if a0==0, jump to loop%d's .L.end.%d segment\n",
			       C, C);
			printf("  beqz a0, .L.end.%d\n", C);
		}
		// gen loop's body
		printf("\n# then statement%d\n", C);
		gen_stmt(node->then_);
		// process loop's increase stmt
		if (node->increase) {
			printf("\n# increase statement%d\n", C);
			gen_expr(node->increase);
		}
		// jump to loop's head
		printf("  # jump to loop%d's .L.begin.%d segment\n", C, C);
		printf("  j .L.begin.%d\n", C);
		// output the loop's tail label
		printf("\n# loop%d's .L.end.%d segment label\n", C, C);
		printf(".L.end.%d:\n", C);
		return;
	}
	case ND_BLOCK:
		for (struct AstNode *nd = node->body; nd; nd = nd->next)
			gen_stmt(nd);
		return;
	case ND_RETURN:
		printf("# terurn statement\n");
		gen_expr(node->lhs);
		// unconditional jump statement, jump to the .L.return segment
		// 'j offset' is an alias instruction for 'jal x0, offset'
		printf("  # jump to .L.return.%s segment\n", CurFn->name);
		printf("  j .L.return.%s\n", CurFn->name);
		return;
	case ND_EXPR_STMT:
		gen_expr(node->lhs);
		return;
	default:
		break;
	}

	error_token(node->tok, "invalid statement");
}

// cau the offset according to global var list
static void assign_lvar_offsets(struct Function *prog)
{
	// calculate the stack space that needed for every func
	for (struct Function *fn = prog; fn; fn = fn->next) {
		int offset = 0;
		// read all var
		for (struct Local_Var *var = fn->locals; var; var = var->next) {
			// alloc 8 bits to every var
			offset += 8;
			// assign a offset to every var, aka address in stack
			var->offset = -offset;
		}
		// align stack to 16 bits
		fn->stack_size = align_to(offset, 16);
	}
}

void codegen(struct Function *prog)
{
	assign_lvar_offsets(prog);
	for (struct Function *fn = prog; fn; fn = fn->next) {
		printf("\n  # define global %s seg\n", fn->name);
		printf("  .globl %s\n", fn->name);
		printf("# =====%s start=====\n", fn->name);
		printf("# %s segment label\n", fn->name);
		printf("%s:\n", fn->name);
		CurFn = fn;

		// stack layout
		//-------------------------------// sp
		//              ra
		//-------------------------------// ra = sp-8
		//              fp
		//-------------------------------// fp = sp-16
		//              var
		//-------------------------------// sp = sp-16-StackSize
		//          express cau
		//-------------------------------//

		// prologue
		// push ra, store val of ra
		printf("  # push ra, store val of ra");
		printf("  addi sp, sp, -16\n");
		printf("  sd ra, 8(sp)\n");
		// push fp, store val of fp
		printf("  # Stack frame protection\n");
		printf("  sd fp, 0(sp)\n");
		// write sp into fp
		printf("  mv fp, sp\n");

		// offset is the stack usable size
		printf("  # Allocate stack space\n");
		printf("  addi sp, sp, -%d\n", prog->stack_size);

		// Iterate through statements list, gen code
		printf("\n# =====%s segment body=====\n", fn->name);
		gen_stmt(fn->body);
		assert(StackDepth == 0);

		// epilogue
		// output return seg label
		printf("\n# =====%s segment end=====\n", fn->name);
		printf("# return segment label\n");
		printf(".L.return.%s:\n", fn->name);
		// write back fp into sp
		printf("  # return value, recovering stack frames\n");
		printf("  mv sp, fp\n");
		// pop fp, recover fp
		printf("  ld fp, 0(sp)\n");
		// pop ra, recover ra
		printf("  ld ra, 8(sp)\n");
		printf("  addi sp, sp, 16\n");

		printf("  ret\n");
	}
}
