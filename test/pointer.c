#include "test.h"

int main()
{
	// [20] Support unary '&' '*' pointer operators
	printf("[20] Support unary '&' '*' pointer operators\n");
	ASSERT(3, ({
		       int x = 3;
		       *&x;
	       }));
	ASSERT(3, ({
		       int x = 3;
		       int *y = &x;
		       int **z = &y;
		       **z;
	       }));
	ASSERT(5, ({
		       int x = 3;
		       int y = 5;
		       *(&x + 1);
	       }));
	ASSERT(3, ({
		       int x = 3;
		       int y = 5;
		       *(&y - 1);
	       }));
	ASSERT(5, ({
		       int x = 3;
		       int y = 5;
		       *(&x - (-1));
	       }));
	ASSERT(5, ({
		       int x = 3;
		       int *y = &x;
		       *y = 5;
		       x;
	       }));
	ASSERT(7, ({
		       int x = 3;
		       int y = 5;
		       *(&x + 1) = 7;
		       y;
	       }));
	ASSERT(7, ({
		       int x = 3;
		       int y = 5;
		       *(&y - 2 + 1) = 7;
		       x;
	       }));
	ASSERT(5, ({
		       int x = 3;
		       (&x + 2) - &x + 3;
	       }));
	ASSERT(8, ({
		       int x, y;
		       x = 3;
		       y = 5;
		       x + y;
	       }));
	ASSERT(8, ({
		       int x = 3, y = 5;
		       x + y;
	       }));

	// [27] Support for one-dimensional arrays
	printf("[27] Support for one-dimensional arrays\n");
	ASSERT(3, ({
		       int x[2];
		       int *y = &x;
		       *y = 3;
		       *x;
	       }));

	ASSERT(3, ({
		       int x[3];
		       *x = 3;
		       *(x + 1) = 4;
		       *(x + 2) = 5;
		       *x;
	       }));
	ASSERT(4, ({
		       int x[3];
		       *x = 3;
		       *(x + 1) = 4;
		       *(x + 2) = 5;
		       *(x + 1);
	       }));
	ASSERT(5, ({
		       int x[3];
		       *x = 3;
		       *(x + 1) = 4;
		       *(x + 2) = 5;
		       *(x + 2);
	       }));

	// [28] Support for multi-dimensional arrays
	printf("[28] Support for multi-dimensional arrays\n");
	ASSERT(0, ({
		       int x[2][3];
		       int *y = x;
		       *y = 0;
		       **x;
	       }));
	ASSERT(1, ({
		       int x[2][3];
		       int *y = x;
		       *(y + 1) = 1;
		       *(*x + 1);
	       }));
	ASSERT(2, ({
		       int x[2][3];
		       int *y = x;
		       *(y + 2) = 2;
		       *(*x + 2);
	       }));
	ASSERT(3, ({
		       int x[2][3];
		       int *y = x;
		       *(y + 3) = 3;
		       **(x + 1);
	       }));
	ASSERT(4, ({
		       int x[2][3];
		       int *y = x;
		       *(y + 4) = 4;
		       *(*(x + 1) + 1);
	       }));
	ASSERT(5, ({
		       int x[2][3];
		       int *y = x;
		       *(y + 5) = 5;
		       *(*(x + 1) + 2);
	       }));

	ASSERT(3, ({
		       int x[3];
		       *x = 3;
		       x[1] = 4;
		       x[2] = 5;
		       *x;
	       }));
	ASSERT(4, ({
		       int x[3];
		       *x = 3;
		       x[1] = 4;
		       x[2] = 5;
		       *(x + 1);
	       }));
	ASSERT(5, ({
		       int x[3];
		       *x = 3;
		       x[1] = 4;
		       x[2] = 5;
		       *(x + 2);
	       }));
	ASSERT(5, ({
		       int x[3];
		       *x = 3;
		       x[1] = 4;
		       x[2] = 5;
		       *(x + 2);
	       }));
	ASSERT(5, ({
		       int x[3];
		       *x = 3;
		       x[1] = 4;
		       2 [x] = 5;
		       *(x + 2);
	       }));

	// [29] Support for '[]' operator
	printf("[29] Support for '[]' operator\n");
	ASSERT(0, ({
		       int x[2][3];
		       int *y = x;
		       y[0] = 0;
		       x[0][0];
	       }));
	ASSERT(1, ({
		       int x[2][3];
		       int *y = x;
		       y[1] = 1;
		       x[0][1];
	       }));
	ASSERT(2, ({
		       int x[2][3];
		       int *y = x;
		       y[2] = 2;
		       x[0][2];
	       }));
	ASSERT(3, ({
		       int x[2][3];
		       int *y = x;
		       y[3] = 3;
		       x[1][0];
	       }));
	ASSERT(4, ({
		       int x[2][3];
		       int *y = x;
		       y[4] = 4;
		       x[1][1];
	       }));
	ASSERT(5, ({
		       int x[2][3];
		       int *y = x;
		       y[5] = 5;
		       x[1][2];
	       }));

	printf("OK\n");
	return 0;
}
