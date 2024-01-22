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

	printf("OK\n");
	return 0;
}
