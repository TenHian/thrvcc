#include "test.h"

int ret3()
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

int *g1_ptr()
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

	printf("OK\n");
	return 0;
}
