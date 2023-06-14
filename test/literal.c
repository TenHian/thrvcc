#include "test.h"

int main()
{
	// [72] Support for char literals
	printf("[72] Support for char literals\n");
	ASSERT(97, 'a');
	ASSERT(10, '\n');
	ASSERT(127, '\x7f');

	printf("OK\n");
	return 0;
}
