#include "test.h"

int main()
{
	// [15] Support 'if' statement
	printf("[15] Support 'if' statement\n");
	ASSERT(3, ({
		       int x;
		       if (0)
			       x = 2;
		       else
			       x = 3;
		       x;
	       }));
	ASSERT(3, ({
		       int x;
		       if (1 - 1)
			       x = 2;
		       else
			       x = 3;
		       x;
	       }));
	ASSERT(2, ({
		       int x;
		       if (1)
			       x = 2;
		       else
			       x = 3;
		       x;
	       }));
	ASSERT(2, ({
		       int x;
		       if (2 - 1)
			       x = 2;
		       else
			       x = 3;
		       x;
	       }));

	// [16] Support 'for' statement
	printf("[16] Support 'for' statement\n");
	ASSERT(55, ({
		       int i = 0;
		       int j = 0;
		       for (i = 0; i <= 10; i = i + 1)
			       j = i + j;
		       j;
	       }));

	// [17] Support 'while' statement
	printf("[17] Support 'while' statement\n");
	ASSERT(10, ({
		       int i = 0;
		       while (i < 10)
			       i = i + 1;
		       i;
	       }));

	// [13] Support {...}
	printf("[13] Support {...}\n");
	ASSERT(3, ({
		       1;
		       {
			       2;
		       }
		       3;
	       }));

	// [14] Support empty statement
	printf("[14] Support empty statement\n");
	ASSERT(5, ({
		       ;
		       ;
		       ;
		       5;
	       }));
	ASSERT(10, ({
		       int i = 0;
		       while (i < 10)
			       i = i + 1;
		       i;
	       }));
	ASSERT(55, ({
		       int i = 0;
		       int j = 0;
		       while (i <= 10) {
			       j = i + j;
			       i = i + 1;
		       }
		       j;
	       }));

	// [47] Support ',' operator
	printf("[47] Support ',' operator\n");
	ASSERT(3, (1, 2, 3));
	ASSERT(5, ({
		       int i = 2, j = 3;
		       (i = 5, j) = 6;
		       i;
	       }));
	ASSERT(6, ({
		       int i = 2, j = 3;
		       (i = 5, j) = 6;
		       j;
	       }));

	// [75] Support for defining local variables in the loop scope
	printf("[75] Support for defining local variables in the loop scope\n");
	ASSERT(55, ({
		       int j = 0;
		       for (int i = 0; i <= 10; i = i + 1)
			       j = j + i;
		       j;
	       }));
	ASSERT(3, ({
		       int i = 3;
		       int j = 0;
		       for (int i = 0; i <= 10; i = i + 1)
			       j = j + i;
		       i;
	       }));

	// [84] Support for '&&' and '||' operator
	printf("[84] Support for '&&' and '||' operator\n");
	ASSERT(1, 0 || 1);
	ASSERT(1, 0 || (2 - 2) || 5);
	ASSERT(0, 0 || 0);
	ASSERT(0, 0 || (2 - 2));

	ASSERT(0, 0 && 1);
	ASSERT(0, (2 - 2) && 5);
	ASSERT(1, 1 && 5);

	// [88] Support for 'goto' and label statements
	printf("[88] Support for 'goto' and label statements\n");
	ASSERT(3, ({
		       int i = 0;
		       goto a;
       a:
		       i++;
       b:
		       i++;
       c:
		       i++;
		       i;
	       }));
	ASSERT(2, ({
		       int i = 0;
		       goto e;
       d:
		       i++;
       e:
		       i++;
       f:
		       i++;
		       i;
	       }));
	ASSERT(1, ({
		       int i = 0;
		       goto i;
       g:
		       i++;
       h:
		       i++;
       i:
		       i++;
		       i;
	       }));

	// [89] Resolve conflicts between typedef and label
	printf("[89] Resolve conflicts between typedef and label\n");
	ASSERT(1, ({
		       typedef int foo;
		       goto foo;
       foo:;
		       1;
	       }));

	// [90] Support for 'break' statement
	printf("[90] Support for 'break' statement\n");
	ASSERT(3, ({
		       int i = 0;
		       for (; i < 10; i++) {
			       if (i == 3)
				       break;
		       }
		       i;
	       }));
	ASSERT(4, ({
		       int i = 0;
		       while (1) {
			       if (i++ == 3)
				       break;
		       }
		       i;
	       }));
	ASSERT(3, ({
		       int i = 0;
		       for (; i < 10; i++) {
			       for (;;)
				       break;
			       if (i == 3)
				       break;
		       }
		       i;
	       }));
	ASSERT(4, ({
		       int i = 0;
		       while (1) {
			       while (1)
				       break;
			       if (i++ == 3)
				       break;
		       }
		       i;
	       }));

	// [91] Support for 'continue' statement
	printf("[91] Support for 'continue' statement\n");
	ASSERT(10, ({
		       int i = 0;
		       int j = 0;
		       for (; i < 10; i++) {
			       if (i > 5)
				       continue;
			       j++;
		       }
		       i;
	       }));
	ASSERT(6, ({
		       int i = 0;
		       int j = 0;
		       for (; i < 10; i++) {
			       if (i > 5)
				       continue;
			       j++;
		       }
		       j;
	       }));
	ASSERT(10, ({
		       int i = 0;
		       int j = 0;
		       for (; !i;) {
			       for (; j != 10; j++)
				       continue;
			       break;
		       }
		       j;
	       }));
	ASSERT(11, ({
		       int i = 0;
		       int j = 0;
		       while (i++ < 10) {
			       if (i > 5)
				       continue;
			       j++;
		       }
		       i;
	       }));
	ASSERT(5, ({
		       int i = 0;
		       int j = 0;
		       while (i++ < 10) {
			       if (i > 5)
				       continue;
			       j++;
		       }
		       j;
	       }));
	ASSERT(11, ({
		       int i = 0;
		       int j = 0;
		       while (!i) {
			       while (j++ != 10)
				       continue;
			       break;
		       }
		       j;
	       }));

	// [92] Support for 'switch' and 'case' statement
	printf("[92] Support for 'switch' and 'case' statement\n");
	ASSERT(5, ({
		       int i = 0;
		       switch (0) {
		       case 0:
			       i = 5;
			       break;
		       case 1:
			       i = 6;
			       break;
		       case 2:
			       i = 7;
			       break;
		       }
		       i;
	       }));
	ASSERT(6, ({
		       int i = 0;
		       switch (1) {
		       case 0:
			       i = 5;
			       break;
		       case 1:
			       i = 6;
			       break;
		       case 2:
			       i = 7;
			       break;
		       }
		       i;
	       }));
	ASSERT(7, ({
		       int i = 0;
		       switch (2) {
		       case 0:
			       i = 5;
			       break;
		       case 1:
			       i = 6;
			       break;
		       case 2:
			       i = 7;
			       break;
		       }
		       i;
	       }));
	ASSERT(0, ({
		       int i = 0;
		       switch (3) {
		       case 0:
			       i = 5;
			       break;
		       case 1:
			       i = 6;
			       break;
		       case 2:
			       i = 7;
			       break;
		       }
		       i;
	       }));
	ASSERT(5, ({
		       int i = 0;
		       switch (0) {
		       case 0:
			       i = 5;
			       break;
		       default:
			       i = 7;
		       }
		       i;
	       }));
	ASSERT(7, ({
		       int i = 0;
		       switch (1) {
		       case 0:
			       i = 5;
			       break;
		       default:
			       i = 7;
		       }
		       i;
	       }));
	ASSERT(2, ({
		       int i = 0;
		       switch (1) {
		       case 0:
			       0;
		       case 1:
			       0;
		       case 2:
			       0;
			       i = 2;
		       }
		       i;
	       }));
	ASSERT(0, ({
		       int i = 0;
		       switch (3) {
		       case 0:
			       0;
		       case 1:
			       0;
		       case 2:
			       0;
			       i = 2;
		       }
		       i;
	       }));

	ASSERT(3, ({
		       int i = 0;
		       switch (-1) {
		       case 0xffffffff:
			       i = 3;
			       break;
		       }
		       i;
	       }));

	// [123] support for 'do while' stmt
	printf("[123] support for 'do while' stmt\n");
	ASSERT(7, ({
		       int i = 0;
		       int j = 0;
		       do {
			       j++;
		       } while (i++ < 6);
		       j;
	       }));
	ASSERT(4, ({
		       int i = 0;
		       int j = 0;
		       int k = 0;
		       do {
			       if (++j > 3)
				       break;
			       continue;
			       k++;
		       } while (1);
		       j;
	       }));

	printf("OK\n");
	return 0;
}
