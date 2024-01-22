int assert(int expected, int actual, char *code);
int printf(char *fmt, ...);
int sprintf(char *buf, char *fmt, ...);
int strcmp(char *p, char *q);
int memcmp(char *p, char *q, long n);

// [157] support empty directive
#

/* */ #

int main()
{
	printf("OK\n");
	return 0;
}
