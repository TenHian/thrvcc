#include "test.h"

int main()
{
	// [15] Support 'if' statement
	printf("[15] Support 'if' statement\n");
	ASSERT(3, ({
		       int x;
		       if (0)
			       x = 2;
		       else
			       x = 3;
		       x;
	       }));
	ASSERT(3, ({
		       int x;
		       if (1 - 1)
			       x = 2;
		       else
			       x = 3;
		       x;
	       }));
	ASSERT(2, ({
		       int x;
		       if (1)
			       x = 2;
		       else
			       x = 3;
		       x;
	       }));
	ASSERT(2, ({
		       int x;
		       if (2 - 1)
			       x = 2;
		       else
			       x = 3;
		       x;
	       }));

	// [16] Support 'for' statement
	printf("[16] Support 'for' statement\n");
	ASSERT(55, ({
		       int i = 0;
		       int j = 0;
		       for (i = 0; i <= 10; i = i + 1)
			       j = i + j;
		       j;
	       }));

	// [17] Support 'while' statement
	printf("[17] Support 'while' statement\n");
	ASSERT(10, ({
		       int i = 0;
		       while (i < 10)
			       i = i + 1;
		       i;
	       }));

	// [13] Support {...}
	printf("[13] Support {...}\n");
	ASSERT(3, ({
		       1;
		       {
			       2;
		       }
		       3;
	       }));

	// [14] Support empty statement
	printf("[14] Support empty statement\n");
	ASSERT(5, ({
		       ;
		       ;
		       ;
		       5;
	       }));
	ASSERT(10, ({
		       int i = 0;
		       while (i < 10)
			       i = i + 1;
		       i;
	       }));
	ASSERT(55, ({
		       int i = 0;
		       int j = 0;
		       while (i <= 10) {
			       j = i + j;
			       i = i + 1;
		       }
		       j;
	       }));

	// [47] Support ',' operator
	printf("[47] Support ',' operator\n");
	ASSERT(3, (1, 2, 3));
	ASSERT(5, ({
		       int i = 2, j = 3;
		       (i = 5, j) = 6;
		       i;
	       }));
	ASSERT(6, ({
		       int i = 2, j = 3;
		       (i = 5, j) = 6;
		       j;
	       }));

	// [75] Support for defining local variables in the loop scope
	printf("[75] Support for defining local variables in the loop scope\n");
	ASSERT(55, ({
		       int j = 0;
		       for (int i = 0; i <= 10; i = i + 1)
			       j = j + i;
		       j;
	       }));
	ASSERT(3, ({
		       int i = 3;
		       int j = 0;
		       for (int i = 0; i <= 10; i = i + 1)
			       j = j + i;
		       i;
	       }));

	// [84] Support for '&&' and '||' operator
	printf("[84] Support for '&&' and '||' operator\n");
	ASSERT(1, 0 || 1);
	ASSERT(1, 0 || (2 - 2) || 5);
	ASSERT(0, 0 || 0);
	ASSERT(0, 0 || (2 - 2));

	ASSERT(0, 0 && 1);
	ASSERT(0, (2 - 2) && 5);
	ASSERT(1, 1 && 5);

	printf("OK\n");
	return 0;
}
