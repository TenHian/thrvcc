#include "thrvcc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
int align_to(int N, int Align)
{
	// (0, align] return align
	return (N + Align - 1) / Align * Align;
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
	case ND_COMMA:
		gen_expr(node->lhs);
		gen_addr(node->rhs);
		return;
	case ND_MEMBER:
		gen_addr(node->lhs);
		println("  # calculate offset of struct members");
		println("  li t0, %d", node->member->offset);
		println("  add a0, a0, t0");
		return;
	default:
		break;
	}
	error_token(node->tok, "not an lvalue");
}

// load the value that a0 point to
static void load(struct Type *type)
{
	if (type->kind == TY_ARRAY || type->kind == TY_STRUCT ||
	    type->kind == TY_UNION)
		return;

	println("  # read the addr that sotred in a0, mov its value into a0");
	if (type->size == 1)
		println("  lb a0, 0(a0)");
	else if (type->size == 2)
		println("  lh a0, 0(a0)");
	else if (type->size == 4)
		println("  lw a0, 0(a0)");
	else
		println("  ld a0, 0(a0)");
}

// store the top item (its addr) into a0
static void store(struct Type *type)
{
	pop("a1");

	if (type->kind == TY_STRUCT || type->kind == TY_UNION) {
		println("  # assign for %s",
			type->kind == TY_STRUCT ? "struct" : "union");
		for (int i = 0; i < type->size; ++i) {
			println("  li t0, %d", i);
			println("  add t0, a0, t0");
			println("  lb t1, 0(t0)");
			println("  li t0, %d", i);
			println("  add t0, a1, t0");
			println("  sb t1, 0(t0)");
		}
		return;
	}

	println("  # write the value that stored in a0 into the address stored in a1");
	if (type->size == 1)
		println("  sb a0, 0(a1)");
	else if (type->size == 2)
		println("  sh a0, 0(a1)");
	else if (type->size == 4)
		println("  sw a0, 0(a1)");
	else
		println("  sd a0, 0(a1)");
}

// type enum
enum { I8, I16, I32, I64 };

// Get the enumeration value corresponding to the type
static int get_typeid(struct Type *type)
{
	switch (type->kind) {
	case TY_CHAR:
		return I8;
	case TY_SHORT:
		return I16;
	case TY_INT:
		return I32;
	default:
		return I64;
	}
}

// Type Mapping Table
// The conversion of a 64-bit signed number to a 64-N-bit signed number \
// is achieved by first shifting logically left by N bits \
// and then arithmetically right by N bits
static char i64i8[] = "  # cast to i8 type\n"
		      "  slli a0, a0, 56\n"
		      "  srai a0, a0, 56";
static char i64i16[] = "  # cast to i16 type\n"
		       "  slli a0, a0, 48\n"
		       "  srai a0, a0, 48";
static char i64i32[] = "  # cast to i32 type\n"
		       "  slli a0, a0, 32\n"
		       "  srai a0, a0, 32";

// All type conversion table
static char *CastTable[10][10] = {
	// clang-format off
	
	// be mapped to
	// {i8,  i16,    i32,    i64}
	{NULL,   NULL,   NULL,   NULL}, // cast from i8
	{i64i8,  NULL,   NULL,   NULL}, // cast from i16
	{i64i8,  i64i16, NULL,   NULL}, // cast from i32
	{i64i8,  i64i16, i64i32, NULL}, // cast from i64

	// clang-format on
};

// type conversion(cast)
static void cast(struct Type *from, struct Type *to)
{
	if (to->kind == TY_VOID)
		return;

	if (to->kind == TY_BOOL) {
		println("  # convert to bool type: if 0 assign 0, else assign 1");
		println("  snez a0, a0");
		return;
	}

	// Get the enumerated value of the type
	int _from = get_typeid(from);
	int _to = get_typeid(to);
	if (CastTable[_from][_to]) {
		println("  # cast function");
		println("%s", CastTable[_from][_to]);
	}
}

static void gen_expr(struct AstNode *node)
{
	// .loc, file number, line number
	println("  .loc 1 %d", node->tok->line_no);

	switch (node->kind) {
	case ND_NULL_EXPR:
		return;
	case ND_NUM:
		println("  # load %d into a0", node->val);
		println("  li a0, %ld", node->val);
		return;
	case ND_NEG:
		gen_expr(node->lhs);
		println("  # reverse the value of a0");
		println("  neg%s a0, a0", node->type->size <= 4 ? "w" : "");
		return;
	case ND_VAR:
	case ND_MEMBER:
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
	case ND_COMMA:
		gen_expr(node->lhs);
		gen_expr(node->rhs);
		return;
	case ND_CAST:
		gen_expr(node->lhs);
		cast(node->lhs->type, node->type);
		return;
	case ND_MEMZERO: {
		println("  # zeroized %d bits for %s's mem %d(fp)",
			node->var->type->size, node->var->name,
			node->var->offset);
		// zeroize every byte occupied by a variable on the stack
		for (int i = 0; i < node->var->type->size; i++)
			println("  sb zero, %d(fp)", node->var->offset + i);
		return;
	}
	case ND_COND: {
		int C = count();
		println("\n# ====== conditional operator %d ======", C);
		gen_expr(node->condition);
		println("  # condition determination, if 0, jump");
		println("  beqz a0, .L.else.%d", C);
		gen_expr(node->then_);
		println("  # jump to operator end");
		println("  j .L.end.%d", C);
		println(".L.else.%d:", C);
		gen_expr(node->else_);
		println(".L.end.%d:", C);
		return;
	}
	case ND_NOT:
		gen_expr(node->lhs);
		println("  # NOT operation");
		// if a0=0 set to 1, otherwise 0
		println("  seqz a0, a0");
		return;
	case ND_LOGAND: {
		int C = count();
		println("\n# ====== logical and %d ======", C);
		gen_expr(node->lhs);
		// Determine if it is a short-circuit operation
		println("  # Left short-circuit operation judgment, if 0, then jump");
		println("  beqz a0, .L.false.%d", C);
		gen_expr(node->rhs);
		println("  # The right part of the judgment, 0 will be jumped");
		println("  beqz a0, .L.false.%d", C);
		println("  li a0, 1");
		println("  j .L.end.%d", C);
		println(".L.false.%d:", C);
		println("  li a0, 0");
		println(".L.end.%d:", C);
		return;
	}
	case ND_LOGOR: {
		int C = count();
		println("\n# ====== logical or %d ======", C);
		gen_expr(node->lhs);
		// Determine if it is a short-circuit operation
		println("  # Left short-circuit operation judgment, if 0, then jump");
		println("  bnez a0, .L.true.%d", C);
		gen_expr(node->rhs);
		println("  # The right part of the judgment, 0 will be jumped");
		println("  bnez a0, .L.true.%d", C);
		println("  li a0, 0");
		println("  j .L.end.%d", C);
		println(".L.true.%d:", C);
		println("  li a0, 1");
		println(".L.end.%d:", C);
		return;
	}
	case ND_BITNOT:
		gen_expr(node->lhs);
		println("  # NOT by bit");
		// not a0, a0  equal to  xori a0, a0, -1
		println("  not a0, a0");
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
	char *suffix =
		node->lhs->type->kind == TY_LONG || node->lhs->type->base ? "" :
									    "w";
	switch (node->kind) {
	case ND_ADD: // + a0=a0+a1
		println("  # a0+a1, write the result into a0");
		println("  add%s a0, a0, a1", suffix);
		return;
	case ND_SUB: // - a0=a0-a1
		println("  # a0-a1, write the result into a0");
		println("  sub%s a0, a0, a1", suffix);
		return;
	case ND_MUL: // * a0=a0*a1
		println("  # a0xa1, write the result into a0");
		println("  mul%s a0, a0, a1", suffix);
		return;
	case ND_DIV: // / a0=a0/a1
		println("  # a0/a1, write the result into a0");
		println("  div%s a0, a0, a1", suffix);
		return;
	case ND_MOD:
		println("  # a0 %% a1, write result in a0");
		println("  rem%s a0, a0, a1", suffix);
		return;
	case ND_BITAND:
		println("  # a0 & a1, write result in a0");
		println("  and a0, a0, a1");
		return;
	case ND_BITOR:
		println("  # a0 | a1, write result in a0");
		println("  or a0, a0, a1");
		return;
	case ND_BITXOR:
		println("  # ao ^ a1, write result in a0");
		println("  xor a0, a0, a1");
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
	case ND_SHL:
		println("  # a0 logical-left-shift a1 bits");
		println("  sll%s a0, a0, a1", suffix);
		return;
	case ND_SHR:
		println("  # a0 logical-right-shift a1 bits");
		println("  sra%s a0, a0, a1", suffix);
		return;
	default:
		break;
	}

	error_token(node->tok, "invalid expression");
}

// gen stmt
static void gen_stmt(struct AstNode *node)
{
	// .loc, file number, line number
	println("  .loc 1 %d", node->tok->line_no);

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
			println("  # if a0==0, jump to loop%d's %s segment", C,
				node->brk_label);
			println("  beqz a0, %s", node->brk_label);
		}
		// gen loop's body
		println("\n# then statement%d", C);
		gen_stmt(node->then_);
		// ctue_label
		println("%s:", node->ctue_label);
		// process loop's increase stmt
		if (node->increase) {
			println("\n# increase statement%d", C);
			gen_expr(node->increase);
		}
		// jump to loop's head
		println("  # jump to loop%d's .L.begin.%d segment", C, C);
		println("  j .L.begin.%d", C);
		// output the loop's tail label
		println("\n# loop%d's %s segment label", C, node->brk_label);
		println("%s:", node->brk_label);
		return;
	}
	case ND_SWITCH:
		println("\n# ====== switch statement ======");
		gen_expr(node->condition);

		println("  # Iterate through case label that equal a0's val");
		for (struct AstNode *N = node->case_next; N; N = N->case_next) {
			println("  li t0, %ld", N->val);
			println("  beq a0, t0, %s", N->label);
		}

		if (node->default_case) {
			println("  # jump to default label");
			println("  j %s", node->default_case->label);
		}

		println("  # end switch statement, jump to break label");
		println("  j %s", node->brk_label);
		// generate case label statement
		gen_stmt(node->then_);
		println("# the break label of switch statement, end switch");
		println("%s:", node->brk_label);
		return;
	case ND_CASE:
		println("# case label, its val equal to %ld", node->val);
		println("%s:", node->label);
		gen_stmt(node->lhs);
		return;
	case ND_BLOCK:
		for (struct AstNode *nd = node->body; nd; nd = nd->next)
			gen_stmt(nd);
		return;
	case ND_GOTO:
		println("  j %s", node->unique_label);
		return;
	case ND_LABEL:
		println("%s:", node->unique_label);
		gen_stmt(node->lhs);
		return;
	case ND_RETURN:
		println("# return statement");
		// if not none return stmt
		if (node->lhs)
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
			// Aligning variables
			offset = align_to(offset, var->align);
			// assign a offset to every var, aka address in stack
			var->offset = -offset;
		}
		// align stack to 16 bits
		fn->stack_size = align_to(offset, 16);
	}
}

// given a N, return the value log2N, else return -1
static int log2i(int N)
{
	int log2n = 0;
	if (N <= 0)
		return -1;
	while (N > 1) {
		if (N & 1) {
			return -1;
		}
		N >>= 1;
		log2n++;
	}
	return log2n;
}

static void emit_data(struct Obj_Var *prog)
{
	for (struct Obj_Var *var = prog; var; var = var->next) {
		// skip func or variable that no definition
		if (var->is_function || !var->is_definition)
			continue;

		println("\n  # GLOBAL segment %s", var->name);
		println("  .globl %s", var->name);
		println("  # Aligning global variables");
		if (!var->align)
			error_out("align can not be 0!");
		else if (var->type->align == -1)
			error_out("align can not be -1!");
		println("  .align %d", log2i(var->align));
		// determine if there is a init value
		if (var->init_data) {
			println("\n  # DATA segment label");
			println("  .data");
			println("%s:", var->name);
			struct Relocation *rel = var->rel;
			int position = 0;
			while (position < var->type->size) {
				if (rel && rel->offset == position) {
					// use other variable to initialize
					println("  # global variable %s",
						var->name);
					println("  .quad %s%+ld", rel->label,
						rel->addend);
					rel = rel->next;
					position += 8;
				} else {
					// print string content, include escaped characters
					char c = var->init_data[position++];
					if (isprint(c))
						println("  .byte %d\t# character: %c",
							c, c);
					else
						println("  .byte %d", c);
				}
			}
			continue;
		}

		// .bss segment does not allocate space for the data
		// and only records the amount of space required for the data
		println("  # .BSS, the global not initialized");
		println("  .bss");
		println("%s:", var->name);
		println("  # zero fill %d bytes for global variable",
			var->type->size);
		println("  .zero %d", var->type->size);
	}
}

// push register value into stack
static void reg2stack(int reg, int offset, int size)
{
	println("  # push register %s value into %d(fp) stack", ArgsReg[reg],
		offset);
	switch (size) {
	case 1:
		println("  sb %s, %d(fp)", ArgsReg[reg], offset);
		return;
	case 2:
		println("  sh %s, %d(fp)", ArgsReg[reg], offset);
		return;
	case 4:
		println("  sw %s, %d(fp)", ArgsReg[reg], offset);
		return;
	case 8:
		println("  sd %s, %d(fp)", ArgsReg[reg], offset);
		return;
	}
	unreachable();
}

void emit_text(struct Obj_Var *prog)
{
	// codegen for every single function
	for (struct Obj_Var *fn = prog; fn; fn = fn->next) {
		if (!fn->is_function || !fn->is_definition)
			continue;

		if (fn->is_static) {
			println("\n  # define local %s func", fn->name);
			println("  .local %s", fn->name);
		} else {
			println("\n  # define global %s func", fn->name);
			println("  .globl %s", fn->name);
		}

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
		for (struct Obj_Var *var = fn->params; var; var = var->next)
			reg2stack(i_regs++, var->offset, var->type->size);

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
