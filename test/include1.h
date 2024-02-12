#include "include2.h"

// [186] support __FILE__ and __LINE__
char *include1_filename = __FILE__;
int include1_line = __LINE__;

// [158] support #include "..."
int include1 = 5;
