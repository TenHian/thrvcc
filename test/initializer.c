#include "test.h"

// [104] global variable initializer support for scalars and strings
char g3 = 3;
short g4 = 4;
int g5 = 5;
long g6 = 6;

// [105] global variable initializer support for structures
int g9[3] = { 0, 1, 2 };
struct {
	char a;
	int b;
} g11[2] = { { 1, 2 }, { 3, 4 } };
struct {
	int a[2];
} g12[2] = { { { 1, 2 } } };

// [106] global variable initializer support for union
union {
	int a;
	char b[8];
} g13[2] = { { 0x01020304 }, { 0x05060708 } };
char g17[] = "foobar";
char g18[10] = "foobar";
char g19[3] = "foobar";
char *g20 = g17 + 0;
char *g21 = g17 + 3;
char *g22 = &g17 - 3;
char *g23[] = { g17 + 0, g17 + 3, g17 - 3 };
int g24 = 3;
int *g25 = &g24;
int g26[3] = { 1, 2, 3 };
int *g27 = g26 + 1;
int *g28 = &g11[1].a;
long g29 = (long)(long)g26;
struct {
	struct {
		int a[3];
	} a;
} g30 = { { { 1, 2, 3 } } };
int *g31 = g30.a.a;

// [107] allowed to omit braces inside initializer
union {
	int a;
	char b[8];
} g13_2[2] = { 0x01020304, 0x05060708 };
struct {
	int a[2];
} g40[2] = { { 1, 2 }, 3, 4 };
struct {
	int a[2];
} g41[2] = { 1, 2, 3, 4 };
char g43[][4] = { 'f', 'o', 'o', 0, 'b', 'a', 'r', 0 };

// [108] allowed scalar initialization with extra braces
char *g44 = { "foo" };

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

	// [100] support for omitting array length when an initializer exists
	printf("[100] support for omitting array length when an initializer exists\n");
	ASSERT(4, ({
		       int x[] = { 1, 2, 3, 4 };
		       x[3];
	       }));
	ASSERT(16, ({
		       int x[] = { 1, 2, 3, 4 };
		       sizeof(x);
	       }));
	ASSERT(4, ({
		       char x[] = "foo";
		       sizeof(x);
	       }));

	ASSERT(4, ({
		       typedef char T[];
		       T x = "foo";
		       T y = "x";
		       sizeof(x);
	       }));
	ASSERT(2, ({
		       typedef char T[];
		       T x = "foo";
		       T y = "x";
		       sizeof(y);
	       }));
	ASSERT(2, ({
		       typedef char T[];
		       T x = "x";
		       T y = "foo";
		       sizeof(x);
	       }));
	ASSERT(4, ({
		       typedef char T[];
		       T x = "x";
		       T y = "foo";
		       sizeof(y);
	       }));

	// [101] handling structure initialization for local variables
	printf("[101] handling structure initialization for local variables\n");
	ASSERT(1, ({
		       struct {
			       int a;
			       int b;
			       int c;
		       } x = { 1, 2, 3 };
		       x.a;
	       }));
	ASSERT(2, ({
		       struct {
			       int a;
			       int b;
			       int c;
		       } x = { 1, 2, 3 };
		       x.b;
	       }));
	ASSERT(3, ({
		       struct {
			       int a;
			       int b;
			       int c;
		       } x = { 1, 2, 3 };
		       x.c;
	       }));
	ASSERT(1, ({
		       struct {
			       int a;
			       int b;
			       int c;
		       } x = { 1 };
		       x.a;
	       }));
	ASSERT(0, ({
		       struct {
			       int a;
			       int b;
			       int c;
		       } x = { 1 };
		       x.b;
	       }));
	ASSERT(0, ({
		       struct {
			       int a;
			       int b;
			       int c;
		       } x = { 1 };
		       x.c;
	       }));

	ASSERT(1, ({
		       struct {
			       int a;
			       int b;
		       } x[2] = { { 1, 2 }, { 3, 4 } };
		       x[0].a;
	       }));
	ASSERT(2, ({
		       struct {
			       int a;
			       int b;
		       } x[2] = { { 1, 2 }, { 3, 4 } };
		       x[0].b;
	       }));
	ASSERT(3, ({
		       struct {
			       int a;
			       int b;
		       } x[2] = { { 1, 2 }, { 3, 4 } };
		       x[1].a;
	       }));
	ASSERT(4, ({
		       struct {
			       int a;
			       int b;
		       } x[2] = { { 1, 2 }, { 3, 4 } };
		       x[1].b;
	       }));

	ASSERT(0, ({
		       struct {
			       int a;
			       int b;
		       } x[2] = { { 1, 2 } };
		       x[1].b;
	       }));

	ASSERT(0, ({
		       struct {
			       int a;
			       int b;
		       } x = {};
		       x.a;
	       }));
	ASSERT(0, ({
		       struct {
			       int a;
			       int b;
		       } x = {};
		       x.b;
	       }));

	ASSERT(5, ({
		       typedef struct {
			       int a, b, c, d, e, f;
		       } T;
		       T x = { 1, 2, 3, 4, 5, 6 };
		       T y;
		       y = x;
		       y.e;
	       }));
	ASSERT(2, ({
		       typedef struct {
			       int a, b;
		       } T;
		       T x = { 1, 2 };
		       T y, z;
		       z = y = x;
		       z.b;
	       }));

	// [102] other structures can be used when initializing a structure
	printf("[102] other structures can be used when initializing a structure\n");
	ASSERT(1, ({
		       typedef struct {
			       int a, b;
		       } T;
		       T x = { 1, 2 };
		       T y = x;
		       y.a;
	       }));

	// [103] handling union initialization for local variables
	printf("[103] handling union initialization for local variables\n");
	ASSERT(4, ({
		       union {
			       int a;
			       char b[4];
		       } x = { 0x01020304 };
		       x.b[0];
	       }));
	ASSERT(3, ({
		       union {
			       int a;
			       char b[4];
		       } x = { 0x01020304 };
		       x.b[1];
	       }));

	ASSERT(0x01020304, ({
		       union {
			       struct {
				       char a, b, c, d;
			       } e;
			       int f;
		       } x = { { 4, 3, 2, 1 } };
		       x.f;
	       }));

	// [104] global variable initializer support for scalars and strings
	printf("[104] global variable initializer support for scalars and strings\n");
	ASSERT(3, g3);
	ASSERT(4, g4);
	ASSERT(5, g5);
	ASSERT(6, g6);

	// [105] global variable initializer support for structures
	printf("[105] global variable initializer support for structures\n");
	ASSERT(0, g9[0]);
	ASSERT(1, g9[1]);
	ASSERT(2, g9[2]);

	ASSERT(1, g11[0].a);
	ASSERT(2, g11[0].b);
	ASSERT(3, g11[1].a);
	ASSERT(4, g11[1].b);

	ASSERT(1, g12[0].a[0]);
	ASSERT(2, g12[0].a[1]);
	ASSERT(0, g12[1].a[0]);
	ASSERT(0, g12[1].a[1]);

	// [106] global variable initializer support for union
	printf("[106] global variable initializer support for union\n");
	ASSERT(4, g13[0].b[0]);
	ASSERT(3, g13[0].b[1]);
	ASSERT(8, g13[1].b[0]);
	ASSERT(7, g13[1].b[1]);

	ASSERT(7, sizeof(g17));
	ASSERT(10, sizeof(g18));
	ASSERT(3, sizeof(g19));

	ASSERT(0, memcmp(g17, "foobar", 7));
	ASSERT(0, memcmp(g18, "foobar\0\0\0", 10));
	ASSERT(0, memcmp(g19, "foo", 3));

	ASSERT(0, strcmp(g20, "foobar"));
	ASSERT(0, strcmp(g21, "bar"));
	ASSERT(0, strcmp(g22 + 3, "foobar"));

	ASSERT(0, strcmp(g23[0], "foobar"));
	ASSERT(0, strcmp(g23[1], "bar"));
	ASSERT(0, strcmp(g23[2] + 3, "foobar"));

	ASSERT(3, g24);
	ASSERT(3, *g25);
	ASSERT(2, *g27);
	ASSERT(3, *g28);
	ASSERT(1, *(int *)g29);

	ASSERT(1, g31[0]);
	ASSERT(2, g31[1]);
	ASSERT(3, g31[2]);

	// [107] allowed to omit braces inside initializer
	printf("[107] allowed to omit braces inside initializer\n");
	ASSERT(4, g13_2[0].b[0]);
	ASSERT(3, g13_2[0].b[1]);
	ASSERT(8, g13_2[1].b[0]);
	ASSERT(7, g13_2[1].b[1]);

	ASSERT(1, g40[0].a[0]);
	ASSERT(2, g40[0].a[1]);
	ASSERT(3, g40[1].a[0]);
	ASSERT(4, g40[1].a[1]);

	ASSERT(1, g41[0].a[0]);
	ASSERT(2, g41[0].a[1]);
	ASSERT(3, g41[1].a[0]);
	ASSERT(4, g41[1].a[1]);

	ASSERT(0, ({
		       int x[2][3] = { 0, 1, 2, 3, 4, 5 };
		       x[0][0];
	       }));
	ASSERT(3, ({
		       int x[2][3] = { 0, 1, 2, 3, 4, 5 };
		       x[1][0];
	       }));

	ASSERT(0, ({
		       struct {
			       int a;
			       int b;
		       } x[2] = { 0, 1, 2, 3 };
		       x[0].a;
	       }));
	ASSERT(2, ({
		       struct {
			       int a;
			       int b;
		       } x[2] = { 0, 1, 2, 3 };
		       x[1].a;
	       }));

	ASSERT(0, strcmp(g43[0], "foo"));
	ASSERT(0, strcmp(g43[1], "bar"));

	// [108] allowed scalar initialization with extra braces
	printf("[108] allowed scalar initialization with extra braces\n");
	ASSERT(0, strcmp(g44, "foo"));

	printf("OK\n");
	return 0;
}
