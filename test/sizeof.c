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

	// [130] support for unsigned integer
	printf("[130] support for unsigned integer\n");
	ASSERT(1, sizeof(unsigned char));
	ASSERT(1, sizeof(unsigned char unsigned));

	ASSERT(2, sizeof(unsigned short));
	ASSERT(2, sizeof(int short unsigned));

	ASSERT(4, sizeof(unsigned int));
	ASSERT(4, sizeof(unsigned));
	ASSERT(4, sizeof(unsigned unsigned));

	ASSERT(8, sizeof(unsigned long));
	ASSERT(8, sizeof(unsigned long int));

	ASSERT(8, sizeof(unsigned long long));
	ASSERT(8, sizeof(unsigned long long int));

	ASSERT(1, sizeof((char)1));
	ASSERT(2, sizeof((short)1));
	ASSERT(4, sizeof((int)1));
	ASSERT(8, sizeof((long)1));

	ASSERT(4, sizeof((char)1 + (char)1));
	ASSERT(4, sizeof((short)1 + (short)1));
	ASSERT(4, sizeof(1 ? 2 : 3));
	ASSERT(4, sizeof(1 ? (short)2 : (char)3));
	ASSERT(8, sizeof(1 ? (long)2 : (char)3));

	// [132] replacing int with long or ulong in some expressions
	printf("[132] replacing int with long or ulong in some expressions\n");
	ASSERT(1, sizeof(char) << 31 >> 31);
	ASSERT(1, sizeof(char) << 63 >> 63);

	// [139] support float and double for local variables or type conversions
	printf("[139] support float and double for local variables or type conversions\n");
	ASSERT(4, sizeof(float));
	ASSERT(8, sizeof(double));

	// [141] support '+' '-' '*' '/' for float
	printf("[141] support '+' '-' '*' '/' for float\n");
	ASSERT(4, sizeof(1f + 2));
	ASSERT(8, sizeof(1.0 + 2));
	ASSERT(4, sizeof(1f - 2));
	ASSERT(8, sizeof(1.0 - 2));
	ASSERT(4, sizeof(1f * 2));
	ASSERT(8, sizeof(1.0 * 2));
	ASSERT(4, sizeof(1f / 2));
	ASSERT(8, sizeof(1.0 / 2));

	printf("OK\n");
	return 0;
}
