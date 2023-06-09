#include "test.h"

int main()
{
	// [53] Support for 'union'
	printf("[53] Support for 'union'\n");
	ASSERT(8, ({
		       union {
			       int a;
			       char b[6];
		       } x;
		       sizeof(x);
	       }));
	ASSERT(3, ({
		       union {
			       int a;
			       char b[4];
		       } x;
		       x.a = 515;
		       x.b[0];
	       }));
	ASSERT(2, ({
		       union {
			       int a;
			       char b[4];
		       } x;
		       x.a = 515;
		       x.b[1];
	       }));
	ASSERT(0, ({
		       union {
			       int a;
			       char b[4];
		       } x;
		       x.a = 515;
		       x.b[2];
	       }));
	ASSERT(0, ({
		       union {
			       int a;
			       char b[4];
		       } x;
		       x.a = 515;
		       x.b[3];
	       }));

	// [54] Support for struct and union assignment
	printf("[54] Support for struct and union assignment\n");
	ASSERT(3, ({
		       union {
			       int a, b;
		       } x, y;
		       x.a = 3;
		       y.a = 5;
		       y = x;
		       y.a;
	       }));
	ASSERT(3, ({
		       union {
			       struct {
				       int a, b;
			       } c;
		       } x, y;
		       x.c.b = 3;
		       y.c.b = 5;
		       y = x;
		       y.c.b;
	       }));

	printf("OK\n");
	return 0;
}
