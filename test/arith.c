#include "test.h"

int main()
{
	// [1] return the input val
	printf("[1] return the input val\n");
	ASSERT(0, 0);
	ASSERT(42, 42);
	// [2] support "+, -" operator
	printf("[2] support '+, -' operator\n");
	ASSERT(21, 5 + 20 - 4);
	// [3] support whitespaces
	printf("[3] support whitespaces\n");
	ASSERT(41, 12 + 34 - 5);
	// [5] support * / ()
	printf("[5] support * / ()\n");
	ASSERT(47, 5 + 6 * 7);
	ASSERT(15, 5 * (9 - 6));
	ASSERT(4, (3 + 5) / 2);
	// [6] support unary operators like + -
	printf("[6] support unary operators like + -\n");
	ASSERT(10, - -10);
	ASSERT(10, - -+10);

	// [7] Support Conditional Operators
	printf("[7] Support Conditional Operators\n");
	ASSERT(0, 0 == 1);
	ASSERT(1, 42 == 42);
	ASSERT(1, 0 != 1);
	ASSERT(0, 42 != 42);

	ASSERT(1, 0 < 1);
	ASSERT(0, 1 < 1);
	ASSERT(0, 2 < 1);
	ASSERT(1, 0 <= 1);
	ASSERT(1, 1 <= 1);
	ASSERT(0, 2 <= 1);

	ASSERT(1, 1 > 0);
	ASSERT(0, 1 > 1);
	ASSERT(0, 1 > 2);
	ASSERT(1, 1 >= 0);
	ASSERT(1, 1 >= 1);
	ASSERT(0, 1 >= 2);

	// [67] Implement regular arithmetic conversions
	printf("[67] Implement regular arithmetic conversions\n");
	ASSERT(0, 1073741824 * 100 / 100);

	// [76] Support for '+=' '-=' '*=' '/='
	printf("[76] Support for '+=' '-=' '*=' '/='\n");
	ASSERT(7, ({
		       int i = 2;
		       i += 5;
		       i;
	       }));
	ASSERT(7, ({
		       int i = 2;
		       i += 5;
	       }));
	ASSERT(3, ({
		       int i = 5;
		       i -= 2;
		       i;
	       }));
	ASSERT(3, ({
		       int i = 5;
		       i -= 2;
	       }));
	ASSERT(6, ({
		       int i = 3;
		       i *= 2;
		       i;
	       }));
	ASSERT(6, ({
		       int i = 3;
		       i *= 2;
	       }));
	ASSERT(3, ({
		       int i = 6;
		       i /= 2;
		       i;
	       }));
	ASSERT(3, ({
		       int i = 6;
		       i /= 2;
	       }));

	printf("OK\n");
	return 0;
}
