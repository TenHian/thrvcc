#include "test.h"

int main()
{
	// [64] Support for sizeof on types
	printf("[64] Support for sizeof on types\n");
	ASSERT(1, sizeof(char));
	ASSERT(2, sizeof(short));
	ASSERT(2, sizeof(short int));
	ASSERT(2, sizeof(int short));
	ASSERT(4, sizeof(int));
	ASSERT(8, sizeof(long));
	ASSERT(8, sizeof(long int));
	ASSERT(8, sizeof(long int));
	ASSERT(8, sizeof(long long));
	ASSERT(8, sizeof(long long int));
	ASSERT(8, sizeof(char *));
	ASSERT(8, sizeof(int *));
	ASSERT(8, sizeof(long *));
	ASSERT(8, sizeof(int **));
	ASSERT(8, sizeof(int(*)[4]));
	ASSERT(32, sizeof(int *[4]));
	ASSERT(16, sizeof(int[4]));
	ASSERT(48, sizeof(int[3][4]));
	ASSERT(8, sizeof(struct {
		       int a;
		       int b;
	       }));

	// [67] Implement regular arithmetic conversions
	printf("[67] Implement regular arithmetic conversions\n");
	ASSERT(8, sizeof(-10 + (long)5));
	ASSERT(8, sizeof(-10 - (long)5));
	ASSERT(8, sizeof(-10 * (long)5));
	ASSERT(8, sizeof(-10 / (long)5));
	ASSERT(8, sizeof((long)-10 + 5));
	ASSERT(8, sizeof((long)-10 - 5));
	ASSERT(8, sizeof((long)-10 * 5));
	ASSERT(8, sizeof((long)-10 / 5));

	// [77] Support for prefix increment and decrement operators '++' '--'
	printf("[77] Support for prefix increment and decrement operators '++' '--'\n");
	ASSERT(1, ({
		       char i;
		       sizeof(++i);
	       }));

	// [78] Support for postfix increment and decrement operators '++' '--'
	printf("[78] Support for postfix increment and decrement operators '++' '--'\n");
	ASSERT(1, ({
		       char i;
		       sizeof(i++);
	       }));

	// [85] Add the concept of incomplete array type
	printf("[85] Add the concept of incomplete array type\n");
	ASSERT(8, sizeof(int(*)[10]));
	ASSERT(8, sizeof(int(*)[][10]));

	// [129] support for keyword 'signed'
	printf("[129] support for keyword 'signed'\n");
	ASSERT(1, sizeof(char));
	ASSERT(1, sizeof(signed char));
	ASSERT(1, sizeof(signed char signed));

	ASSERT(2, sizeof(short));
	ASSERT(2, sizeof(int short));
	ASSERT(2, sizeof(short int));
	ASSERT(2, sizeof(signed short));
	ASSERT(2, sizeof(int short signed));

	ASSERT(4, sizeof(int));
	ASSERT(4, sizeof(signed int));
	ASSERT(4, sizeof(signed));
	ASSERT(4, sizeof(signed signed));

	ASSERT(8, sizeof(long));
	ASSERT(8, sizeof(signed long));
	ASSERT(8, sizeof(signed long int));

	ASSERT(8, sizeof(long long));
	ASSERT(8, sizeof(signed long long));
	ASSERT(8, sizeof(signed long long int));

	printf("OK\n");
	return 0;
}
