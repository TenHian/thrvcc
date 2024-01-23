int assert(int expected, int actual, char *code);
int printf(char *fmt, ...);
int sprintf(char *buf, char *fmt, ...);
int strcmp(char *p, char *q);
int memcmp(char *p, char *q, long n);

// [158] support #include "..."
#include "include1.h"

// [157] support empty directive
#

/* */ #

int main()
{
	printf("[158] support #include \"\"\n");
	assert(5, include1, "include1");
	assert(7, include2, "include2");

	printf("[161] support #if and #endif\n");
#if 0
#include "/no/such/file"
	assert(0,1, "1");

	// [162] skip nested #if statements in #if statements with false values
#if nested
#endif
#endif
	int m = 0;

#if 1
	m = 5;
#endif
	assert(5, m, "m");

	printf("[163] support #else\n");
#if 1
#if 0
#if 1
	foo bar
#endif
#endif
	m = 3;
#endif
	assert(3, m, "m");

#if 1 - 1
#if 1
#endif
#if 1
#else
#endif
#if 0
#else
#endif
	m = 2;
#else
#if 1
	m = 3;
#endif
#endif
	assert(3, m, "m");

#if 1
	m = 2;
#else
	m = 3;
#endif
	assert(2, m, "m");

	printf("OK\n");
	return 0;
}
