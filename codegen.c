#include "thrvcc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

// output file
static FILE *OutputFile;
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
static struct Obj_Var *CurFn;

static void gen_expr(struct AstNode *node);
static void gen_stmt(struct AstNode *node);

// output string to target file and line break
static void println(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vfprintf(OutputFile, fmt, va);
	va_end(va);
	fprintf(OutputFile, "\n");
}

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
	println("  # push a0 into the top of the stack");
	println("  addi sp, sp, -8");
	println("  sd a0, 0(sp)");
	StackDepth++;
}

// pop stack, mov the top val of stack into a1
static void pop(char *reg)
{
	println("  # pop out the top of the stack, and mov it into %s", reg);
	println("  ld %s, 0(sp)", reg);
	println("  addi sp, sp, 8");
	StackDepth--;
}

// align to an integer multiple of align
static int align_to(int N, int align)
{
	// (0, align] return align
	return (N + align - 1) / align * align;
}

// calculate the absolute address of the given node
// if error, means node not in menory
static void gen_addr(struct AstNode *node)
{
	switch (node->kind) {
	case ND_VAR:
		if (node->var->is_local) { // offset fp
			println("  # get varable %s's address in stack as %d(fp)",
				node->var->name, node->var->offset);
			println("  addi a0, fp, %d", node->var->offset);
		} else {
			println("  # get global var %s address",
				node->var->name);
			println("  la a0, %s", node->var->name);
		}
		return;
	case ND_DEREF:
		gen_expr(node->lhs);
		return;
	default:
		break;
	}
	error_token(node->tok, "not an lvalue");
}

// load the value that a0 point to
static void load(struct Type *type)
{
	if (type->kind == TY_ARRAY)
		return;

	println("  # read the addr that sotred in a0, mov its value into a0");
	if (type->size == 1)
		println("  lb a0, 0(a0)");
	else
		println("  ld a0, 0(a0)");
}

// store the top item (its addr) into a0
static void store(struct Type *type)
{
	pop("a1");

	println("  # write the value that stored in a0 into the address stored in a1");
	if (type->size == 1)
		println("  sb a0, 0(a1)");
	else
		println("  sd a0, 0(a1)");
}

static void gen_expr(struct AstNode *node)
{
	switch (node->kind) {
	case ND_NUM:
		println("  # load %d into a0", node->val);
		println("  li a0, %d", node->val);
		return;
	case ND_NEG:
		gen_expr(node->lhs);
		println("  # reverse the value of a0");
		println("  neg a0, a0");
		return;
	case ND_VAR:
		gen_addr(node);
		load(node->type);
		return;
	case ND_DEREF:
		gen_expr(node->lhs);
		load(node->type);
		return;
	case ND_ADDR:
		gen_addr(node->lhs);
		return;
	case ND_ASSIGN:
		gen_addr(node->lhs);
		push();
		gen_expr(node->rhs);
		store(node->type);
		return;
	case ND_STMT_EXPR:
		for (struct AstNode *nd = node->body; nd; nd = nd->next)
			gen_stmt(nd);
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
		println("  # func call %s", node->func_name);
		println("  call %s", node->func_name);
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
		println("  # a0+a1, write the result into a0");
		println("  add a0, a0, a1");
		return;
	case ND_SUB: // - a0=a0-a1
		println("  # a0-a1, write the result into a0");
		println("  sub a0, a0, a1");
		return;
	case ND_MUL: // * a0=a0*a1
		println("  # a0xa1, write the result into a0");
		println("  mul a0, a0, a1");
		return;
	case ND_DIV: // / a0=a0/a1
		println("  # a0/a1, write the result into a0");
		println("  div a0, a0, a1");
		return;
	case ND_EQ:
	case ND_NE:
		// a0=a0^a1
		println("  # determine a0%sa1",
			node->kind == ND_EQ ? "=" : "!=");
		println("  xor a0, a0, a1");

		if (node->kind == ND_EQ)
			// a0==a1
			// a0=a0^a1, sltiu a0, a0, 1
			// if 0, mk 1
			println("  seqz a0, a0");
		else
			// a0!=a1
			// a0=a0^a1, sltu a0, x0, a0
			// if not eq to 0, turn it into 1
			println("  snez a0, a0");
		return;
	case ND_LT:
		println("  # determine a0<a1?");
		println("  slt a0, a0, a1");
		return;
	case ND_LE:
		// a0<=a1 equal to
		// a0=a1<a0, a0=a1^1
		println("  #determine a0<=a1");
		println("  slt a0, a1, a0");
		println("  xori a0, a0, 1");
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
		println("\n# =====Branch Statement %d =====", C);
		// gen conditional stmt
		println("\n# Conditional Statement %d", C);
		gen_expr(node->condition);
		// determine condition, if 0 jump to label else
		println("  # if a0 == 0, jump to branch%d's .L.else.%d segment",
			C, C);
		println("  beqz a0, .L.else.%d", C);
		// then stmt
		println("\n# then statement%d", C);
		gen_stmt(node->then_);
		// over, jump to stmt after if stmt
		println("  # jump to branch%d's .L.end.%d segment", C, C);
		println("  j .L.end.%d", C);
		// else block, else block may empty, so output its label
		println("\n# else statement%d", C);
		println("# branch%d's .L.else.%d segment label", C, C);
		println(".L.else.%d:", C);
		// gen else_
		if (node->else_)
			gen_stmt(node->else_);
		println("\n# branch%d's .L.end.%d segment label", C, C);
		println(".L.end.%d:", C);
		return;
	}
	// for stmt or while stmt
	case ND_FOR: {
		// code block count
		int C = count();
		println("\n# =====Loop Statement %d =====", C);
		// gen init stmt
		if (node->init) {
			println("\n# Init Statement%d", C);
			gen_stmt(node->init);
		}
		// output loop's head label
		println("\n# loop%d's .L.begin.%d segment label", C, C);
		println(".L.begin.%d:", C);
		// process loop's conditional stmt
		println("# Conditional Statement %d", C);
		if (node->condition) {
			// gen loop's conditional stmt
			gen_expr(node->condition);
			// determine condition, if 0 jump to loop's end
			println("  # if a0==0, jump to loop%d's .L.end.%d segment",
				C, C);
			println("  beqz a0, .L.end.%d", C);
		}
		// gen loop's body
		println("\n# then statement%d", C);
		gen_stmt(node->then_);
		// process loop's increase stmt
		if (node->increase) {
			println("\n# increase statement%d", C);
			gen_expr(node->increase);
		}
		// jump to loop's head
		println("  # jump to loop%d's .L.begin.%d segment", C, C);
		println("  j .L.begin.%d", C);
		// output the loop's tail label
		println("\n# loop%d's .L.end.%d segment label", C, C);
		println(".L.end.%d:", C);
		return;
	}
	case ND_BLOCK:
		for (struct AstNode *nd = node->body; nd; nd = nd->next)
			gen_stmt(nd);
		return;
	case ND_RETURN:
		println("# return statement");
		gen_expr(node->lhs);
		// unconditional jump statement, jump to the .L.return segment
		// 'j offset' is an alias instruction for 'jal x0, offset'
		println("  # jump to .L.return.%s segment", CurFn->name);
		println("  j .L.return.%s", CurFn->name);
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
static void assign_lvar_offsets(struct Obj_Var *prog)
{
	// calculate the stack space that needed for every func
	for (struct Obj_Var *fn = prog; fn; fn = fn->next) {
		// if not a function, end!
		if (!fn->is_function)
			continue;

		int offset = 0;
		// read all var
		for (struct Obj_Var *var = fn->locals; var; var = var->next) {
			// alloc space to every var
			offset += var->type->size;
			// assign a offset to every var, aka address in stack
			var->offset = -offset;
		}
		// align stack to 16 bits
		fn->stack_size = align_to(offset, 16);
	}
}

static void emit_data(struct Obj_Var *prog)
{
	for (struct Obj_Var *var = prog; var; var = var->next) {
		if (var->is_function)
			continue;

		println("\n  # DATA segment label");
		println("  .data");
		// determine if there is a value
		if (var->init_data) {
			println("%s:", var->name);
			// print string content, include escaped characters
			for (int i = 0; i < var->type->size; ++i) {
				char c = var->init_data[i];
				if (isprint(c))
					println("  .byte %d\t# character: %c",
						c, c);
				else
					println("  .byte %d", c);
			}
		} else {
			println("\n  # global varable %s", var->name);
			println("  .global %s", var->name);
			println("%s:", var->name);
			println("  # zero fill %d bits", var->type->size);
			println("  .zero %d", var->type->size);
		}
	}
}

void emit_text(struct Obj_Var *prog)
{
	// codegen for every single function
	for (struct Obj_Var *fn = prog; fn; fn = fn->next) {
		if (!fn->is_function)
			continue;

		println("\n  # define global %s seg", fn->name);
		println("  .globl %s", fn->name);
		println("  # code segment labels");
		println("  .text");

		println("# =====%s start=====", fn->name);
		println("# %s segment label", fn->name);
		println("%s:", fn->name);
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
		println("  # push ra, store val of ra ");
		println("  addi sp, sp, -16");
		println("  sd ra, 8(sp)");
		// push fp, store val of fp
		println("  # Stack frame protection");
		println("  sd fp, 0(sp)");
		// write sp into fp
		println("  mv fp, sp");

		// offset is the stack usable size
		println("  # Allocate stack space");
		println("  addi sp, sp, -%d", fn->stack_size);

		int i_regs = 0;
		for (struct Obj_Var *var = fn->params; var; var = var->next) {
			println("  # push reg %s value into %s stack address",
				ArgsReg[i_regs], var->name);
			if (var->type->size == 1)
				println("  sb %s, %d(fp)", ArgsReg[i_regs++],
					var->offset);
			else
				println("  sd %s, %d(fp)", ArgsReg[i_regs++],
					var->offset);
		}

		// Iterate through statements list, gen code
		println("# =====%s segment body=====", fn->name);
		gen_stmt(fn->body);
		assert(StackDepth == 0);

		// epilogue
		// output return seg label
		println("# =====%s segment end=====", fn->name);
		println("# return segment label");
		println(".L.return.%s:", fn->name);
		// write back fp into sp
		println("  # return value, recovering stack frames");
		println("  mv sp, fp");
		// pop fp, recover fp
		println("  ld fp, 0(sp)");
		// pop ra, recover ra
		println("  ld ra, 8(sp)");
		println("  addi sp, sp, 16");

		println("  ret");
	}
}

void codegen(struct Obj_Var *prog, FILE *out)
{
	// set target file stream ptr
	OutputFile = out;
	// calculate Obj_Var offset
	assign_lvar_offsets(prog);
	// gen data
	emit_data(prog);
	// gen code
	emit_text(prog);
}
