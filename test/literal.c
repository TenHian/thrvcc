#include "test.h"

int main()
{
	// [72] Support for char literals
	printf("[72] Support for char literals\n");
	ASSERT(97, 'a');
	ASSERT(10, '\n');
	ASSERT(127, '\x7f');

	// [79] Supports hexadecimal, octal, and binary digital literals
	printf("[79] Supports hexadecimal, octal, and binary digital literals\n");
	ASSERT(511, 0777);
	ASSERT(0, 0x0);
	ASSERT(10, 0xa);
	ASSERT(10, 0XA);
	ASSERT(48879, 0xbeef);
	ASSERT(48879, 0xBEEF);
	ASSERT(48879, 0XBEEF);
	ASSERT(0, 0b0);
	ASSERT(1, 0b1);
	ASSERT(47, 0b101111);
	ASSERT(47, 0B101111);

	printf("OK\n");
	return 0;
}
