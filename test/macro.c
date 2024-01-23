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

	printf("OK\n");
	return 0;
}
