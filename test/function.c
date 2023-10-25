#include "test.h"

int ret3(void)
{
	return 3;
	return 5;
}

int add2(int x, int y)
{
	return x + y;
}

int sub2(int x, int y)
{
	return x - y;
}

int add6(int a, int b, int c, int d, int e, int f)
{
	return a + b + c + d + e + f;
}

int addx(int *x, int y)
{
	return *x + y;
}

int sub_char(char a, char b, char c)
{
	return a - b - c;
}

int fib(int x)
{
	if (x <= 1)
		return 1;
	return fib(x - 1) + fib(x - 2);
}

long sub_long(long a, long b, long c)
{
	return a - b - c;
}

int sub_short(short a, short b, short c)
{
	return a - b - c;
}

int g1;

int *g1_ptr(void)
{
	return &g1;
}
char int_to_char(int x)
{
	return x;
}

int div_long(long a, long b)
{
	return a / b;
}

_Bool bool_fn_add(_Bool x)
{
	return x + 1;
}
_Bool bool_fn_sub(_Bool x)
{
	return x - 1;
}

static int static_fn(void)
{
	return 3;
}

int param_decay(int x[])
{
	return x[0];
}

int counter()
{
	static int i;
	static int j = 1 + 1;
	return i++ + j++;
}

void ret_none()
{
	return;
}

_Bool true_fn();
_Bool false_fn();
char char_fn();
short short_fn();

int add_all(int n, ...);

int main()
{
	// [25] Support for zero-parameter function definitions
	printf("[25] Support for zero-parameter function definitions\n");
	ASSERT(3, ret3());
	// [26] Supports function definitions with up to 6 parameters
	printf("[26] Supports function definitions with up to 6 parameters\n");
	ASSERT(8, add2(3, 5));
	ASSERT(2, sub2(5, 3));
	ASSERT(21, add6(1, 2, 3, 4, 5, 6));
	ASSERT(66, add6(1, 2, add6(3, 4, 5, 6, 7, 8), 9, 10, 11));
	ASSERT(136, add6(1, 2, add6(3, add6(4, 5, 6, 7, 8, 9), 10, 11, 12, 13),
			 14, 15, 16));

	ASSERT(7, add2(3, 4));
	ASSERT(1, sub2(4, 3));
	ASSERT(55, fib(9));

	ASSERT(1, ({ sub_char(7, 3, 3); }));

	// [56] Support for long type
	printf("[56] Support for long type\n");
	ASSERT(2147483648, sub_long(2147483650, 1, 1));

	// [57] Support for short type
	printf("[57] Support for short type\n");
	ASSERT(1, sub_short(4, 2, 1));

	// [69] Process return type conversion
	printf("[69] Process return type conversion\n");
	g1 = 3;

	ASSERT(3, *g1_ptr());
	ASSERT(5, int_to_char(261));

	// [70] Process function argument type conversions
	printf("[70] Process function argument type conversions\n");
	ASSERT(-5, div_long(-10, 2));

	// [71] Support for _Bool type
	printf("[71] Support for _Bool type\n");
	ASSERT(1, bool_fn_add(3));
	ASSERT(0, bool_fn_sub(3));
	ASSERT(1, bool_fn_add(-3));
	ASSERT(0, bool_fn_sub(-3));
	ASSERT(1, bool_fn_add(0));
	ASSERT(1, bool_fn_sub(0));

	// [74] Support static func
	printf("[74] Support static func\n");
	ASSERT(3, static_fn());

	// [86] Decay arrays as pointers in function parameters
	printf("[86] Decay arrays as pointers in function parameters\n");
	ASSERT(3, ({
		       int x[2];
		       x[0] = 3;
		       param_decay(x);
	       }));

	// [113] supports void as a formal parameter
	printf("[113] supports void as a formal parameter\n");
	ASSERT(3, ret3());
	ASSERT(3, *g1_ptr());
	ASSERT(3, static_fn());

	// [119] support for static local variable
	printf("[119] support for static local variable\n");
	ASSERT(2, counter());
	ASSERT(4, counter());
	ASSERT(6, counter());

	// [121] support for none return stmt
	printf("[121] support for none return stmt\n");
	ret_none();

	// [125] support for func return short
	printf("[125] support for func return short\n");
	ASSERT(1, true_fn());
	ASSERT(0, false_fn());
	ASSERT(3, char_fn());
	ASSERT(5, short_fn());

	// [126] support for variadic function call
	printf("[126] support for variadic function call\n");
	ASSERT(6, add_all(3, 1, 2, 3));
	ASSERT(5, add_all(4, 1, 2, 3, -1));

	ASSERT(0, ({
		       char buf[100];
		       sprintf(buf, "%d %d %s", 1, 2, "foo");
		       strcmp("1 2 foo", buf);
	       }));

	printf("OK\n");
	return 0;
}
