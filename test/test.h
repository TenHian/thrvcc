#define ASSERT(x, y) assert(x, y, #y)

// [59] support function declaration
int printf(char *fmt, ...);

// [68] Report an error for an undefined or undeclared function
void assert(int expected, int actual, char *code);

// [106] global variable initializer support for union
int strcmp(char *p, char *q);
int memcmp(char *p, char *q, long n);

// [189] splicing adjacent string literals
int strncmp(char *p, char *q, long n);

// [126] support for variadic function call
int sprintf(char *buf, char *fmt, ...);

// [135] ignore 'const' 'volatile' 'auto' 'register' 'restrict' '_Noreturn'
void exit(int n);
