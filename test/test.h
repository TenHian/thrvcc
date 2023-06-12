#define ASSERT(x, y) assert(x, y, #y)

// [59] support function declaration
int printf();

// [68] Report an error for an undefined or undeclared function
void assert(int expected, int actual, char *code);
