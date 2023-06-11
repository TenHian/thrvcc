#include "test.h"

int main()
{
	// [61] Fix parsing complex type declarations
	printf("[61] Fix parsing complex type declarations\n");
	ASSERT(1, ({
		       char x;
		       sizeof(x);
	       }));
	ASSERT(2, ({
		       short int x;
		       sizeof(x);
	       }));
	ASSERT(2, ({
		       int short x;
		       sizeof(x);
	       }));
	ASSERT(4, ({
		       int x;
		       sizeof(x);
	       }));
	ASSERT(8, ({
		       long int x;
		       sizeof(x);
	       }));
	ASSERT(8, ({
		       int long x;
		       sizeof(x);
	       }));

	printf("OK\n");
	return 0;
}
