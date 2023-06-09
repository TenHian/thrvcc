#include "test.h"

int main()
{
	// [48] Support for 'struct'
	printf("[48] Support for 'struct'\n");
	ASSERT(1, ({
		       struct {
			       int a;
			       int b;
		       } x;
		       x.a = 1;
		       x.b = 2;
		       x.a;
	       }));
	ASSERT(2, ({
		       struct {
			       int a;
			       int b;
		       } x;
		       x.a = 1;
		       x.b = 2;
		       x.b;
	       }));
	ASSERT(1, ({
		       struct {
			       char a;
			       int b;
			       char c;
		       } x;
		       x.a = 1;
		       x.b = 2;
		       x.c = 3;
		       x.a;
	       }));
	ASSERT(2, ({
		       struct {
			       char a;
			       int b;
			       char c;
		       } x;
		       x.b = 1;
		       x.b = 2;
		       x.c = 3;
		       x.b;
	       }));
	ASSERT(3, ({
		       struct {
			       char a;
			       int b;
			       char c;
		       } x;
		       x.a = 1;
		       x.b = 2;
		       x.c = 3;
		       x.c;
	       }));

	ASSERT(0, ({
		       struct {
			       char a;
			       char b;
		       } x[3];
		       char *p = x;
		       p[0] = 0;
		       x[0].a;
	       }));
	ASSERT(1, ({
		       struct {
			       char a;
			       char b;
		       } x[3];
		       char *p = x;
		       p[1] = 1;
		       x[0].b;
	       }));
	ASSERT(2, ({
		       struct {
			       char a;
			       char b;
		       } x[3];
		       char *p = x;
		       p[2] = 2;
		       x[1].a;
	       }));
	ASSERT(3, ({
		       struct {
			       char a;
			       char b;
		       } x[3];
		       char *p = x;
		       p[3] = 3;
		       x[1].b;
	       }));

	ASSERT(6, ({
		       struct {
			       char a[3];
			       char b[5];
		       } x;
		       char *p = &x;
		       x.a[0] = 6;
		       p[0];
	       }));
	ASSERT(7, ({
		       struct {
			       char a[3];
			       char b[5];
		       } x;
		       char *p = &x;
		       x.b[0] = 7;
		       p[3];
	       }));

	ASSERT(6, ({
		       struct {
			       struct {
				       char b;
			       } a;
		       } x;
		       x.a.b = 6;
		       x.a.b;
	       }));

	ASSERT(8, ({
		       struct {
			       int a;
		       } x;
		       sizeof(x);
	       }));
	ASSERT(16, ({
		       struct {
			       int a;
			       int b;
		       } x;
		       sizeof(x);
	       }));
	ASSERT(16, ({
		       struct {
			       int a, b;
		       } x;
		       sizeof(x);
	       }));
	ASSERT(24, ({
		       struct {
			       int a[3];
		       } x;
		       sizeof(x);
	       }));
	ASSERT(32, ({
		       struct {
			       int a;
		       } x[4];
		       sizeof(x);
	       }));
	ASSERT(48, ({
		       struct {
			       int a[3];
		       } x[2];
		       sizeof(x);
	       }));
	ASSERT(2, ({
		       struct {
			       char a;
			       char b;
		       } x;
		       sizeof(x);
	       }));
	ASSERT(0, ({
		       struct {
		       } x;
		       sizeof(x);
	       }));

	// [49] Aligning structure member variables
	printf("[49] Aligning structure member variables\n");
	ASSERT(16, ({
		       struct {
			       char a;
			       int b;
		       } x;
		       sizeof(x);
	       }));
	ASSERT(16, ({
		       struct {
			       int a;
			       char b;
		       } x;
		       sizeof(x);
	       }));

	// [51] Support for structure labels
	printf("[51] Support for structure labels\n");
	ASSERT(16, ({
		       struct t {
			       int a;
			       int b;
		       } x;
		       struct t y;
		       sizeof(y);
	       }));
	ASSERT(16, ({
		       struct t {
			       int a;
			       int b;
		       };
		       struct t y;
		       sizeof(y);
	       }));
	ASSERT(2, ({
		       struct t {
			       char a[2];
		       };
		       {
			       struct t {
				       char a[4];
			       };
		       }
		       struct t y;
		       sizeof(y);
	       }));
	ASSERT(3, ({
		       struct t {
			       int x;
		       };
		       int t = 1;
		       struct t y;
		       y.x = 2;
		       t + y.x;
	       }));

	// [52] Support for '->' operator
	printf("[52] Support for '->' operator\n");
	ASSERT(3, ({
		       struct t {
			       char a;
		       } x;
		       struct t *y = &x;
		       x.a = 3;
		       y->a;
	       }));
	ASSERT(3, ({
		       struct t {
			       char a;
		       } x;
		       struct t *y = &x;
		       y->a = 3;
		       x.a;
	       }));

	printf("OK\n");
	return 0;
}