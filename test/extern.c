#include "test.h"

// [115] Support for keyword 'extern'
extern int ext1;
extern int *ext2;

int main()
{
	// [115] Support for keyword 'extern'
	printf("[115] Support for keyword 'extern'\n");
	ASSERT(5, ext1);
	ASSERT(5, *ext2);

	// [116] process with 'extern' in block aka segment
	printf("[116] process with 'extern' in block aka segment\n");
	extern int ext3;
	ASSERT(7, ext3);

	int ext_fn1(int x);
	ASSERT(5, ext_fn1(5));

	extern int ext_fn2(int x);
	ASSERT(8, ext_fn2(8));

	printf("OK\n");
	return 0;
}
