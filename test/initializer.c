#include "test.h"

int main()
{
	// [96] Support for local variable initializers
	printf("[96] Support for local variable initializers\n");
	ASSERT(1, ({
		       int x[3] = { 1, 2, 3 };
		       x[0];
	       }));
	ASSERT(2, ({
		       int x[3] = { 1, 2, 3 };
		       x[1];
	       }));
	ASSERT(3, ({
		       int x[3] = { 1, 2, 3 };
		       x[2];
	       }));

	ASSERT(2, ({
		       int x[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
		       x[0][1];
	       }));
	ASSERT(4, ({
		       int x[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
		       x[1][0];
	       }));
	ASSERT(6, ({
		       int x[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
		       x[1][2];
	       }));

	// [97] zeroized extra array elements
	printf("[97] zeroized extra array elements\n");
	ASSERT(0, ({
		       int x[3] = {};
		       x[0];
	       }));
	ASSERT(0, ({
		       int x[3] = {};
		       x[1];
	       }));
	ASSERT(0, ({
		       int x[3] = {};
		       x[2];
	       }));

	ASSERT(2, ({
		       int x[2][3] = { { 1, 2 } };
		       x[0][1];
	       }));
	ASSERT(0, ({
		       int x[2][3] = { { 1, 2 } };
		       x[1][0];
	       }));
	ASSERT(0, ({
		       int x[2][3] = { { 1, 2 } };
		       x[1][2];
	       }));

	// [98] skip excess initialization elements
	printf("[98] skip excess initialization elements\n");
	ASSERT(3, ({
		       int x[2][3] = { { 1, 2, 3, 4 }, { 5, 6, 7 } };
		       x[0][2];
	       }));
	ASSERT(5, ({
		       int x[2][3] = { { 1, 2, 3, 4 }, { 5, 6, 7 } };
		       x[1][0];
	       }));

	// [99] supports initialization of string literals
	printf("[99] supports initialization of string literals\n");
	ASSERT('a', ({
		       char x[4] = "abc";
		       x[0];
	       }));
	ASSERT('c', ({
		       char x[4] = "abc";
		       x[2];
	       }));
	ASSERT(0, ({
		       char x[4] = "abc";
		       x[3];
	       }));
	ASSERT('a', ({
		       char x[2][4] = { "abc", "def" };
		       x[0][0];
	       }));
	ASSERT(0, ({
		       char x[2][4] = { "abc", "def" };
		       x[0][3];
	       }));
	ASSERT('d', ({
		       char x[2][4] = { "abc", "def" };
		       x[1][0];
	       }));
	ASSERT('f', ({
		       char x[2][4] = { "abc", "def" };
		       x[1][2];
	       }));

	printf("OK\n");
	return 0;
}
