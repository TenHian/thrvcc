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
	printf("OK\n");
	return 0;
}
