// [176] use the built-in preprocessor for all tests
#include "test.h"

// [158] support #include "..."
#include "include1.h"

// [186] support __FILE__ and __LINE__
char *main_filename1 = __FILE__;
int main_line1 = __LINE__;
#define LINE() __LINE__
int main_line2 = LINE();

// [157] support empty directive
#

/* */ #

// [170] support #define 0-arg macro func
int ret3(void)
{
	return 3;
}

// [173] expand only once in a macro function
int dbl(int x)
{
	return x * x;
}

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

	printf("[164] support #elif\n");
#if 1
	m = 2;
#else
	m = 3;
#endif
	assert(2, m, "m");

#if 0
	m = 1;
#elif 0
	m = 2;
#elif 3 + 5
	m = 3;
#elif 1 * 5
	m = 4;
#endif
	assert(3, m, "m");

#if 1 + 5
	m = 1;
#elif 1
	m = 2;
#elif 3
	m = 2;
#endif
	assert(1, m, "m");

#if 0
  m = 1;
#elif 1
#if 1
	m = 2;
#else
	m = 3;
#endif
#else
	m = 5;
#endif
	assert(2, m, "m");

	printf("[165] support #define\n");
	int M1 = 5;

#define M1 3
	assert(3, M1, "M1");
#define M1 4
	assert(4, M1, "M1");

#define M1 3 + 4 +
	assert(12, M1 5, "5");

#define M1 3 + 4
	assert(23, M1 * 5, "5");

#define ASSERT_ assert(
#define if 5
#define five "5"
#define END )
	ASSERT_ 5, if, five END;

	printf("[166] support #undef\n");
#undef ASSERT_
#undef if
#undef five
#undef END

	if (0)
		;

	printf("[167] expand parameters in #if and #elif\n");
#define M 5
#if M
	m = 5;
#else
	m = 6;
#endif
	assert(5, m, "m");

#define M 5
#if M - 5
	m = 6;
#elif M
	m = 5;
#endif
	assert(5, m, "m");

	printf("[168] macro only expand once\n");
	int M2 = 6;
#define M2 M2 + 3
	assert(9, M2, "M2");

#define M3 M2 + 3
	assert(12, M3, "M3");

	int M4 = 3;
#define M4 M5 * 5
#define M5 M4 + 2
	assert(13, M4, "M4");

	printf("[169] support #ifdef and #ifndef\n");
#ifdef M6
	m = 5;
#else
	m = 3;
#endif
	assert(3, m, "m");

#define M6
#ifdef M6
	m = 5;
#else
	m = 3;
#endif
	assert(5, m, "m");

#ifndef M7
	m = 3;
#else
	m = 5;
#endif
	assert(3, m, "m");

#define M7
#ifndef M7
	m = 3;
#else
	m = 5;
#endif
	assert(5, m, "m");

#if 0
#ifdef NO_SUCH_MACRO
#endif
#ifndef NO_SUCH_MACRO
#endif
#else
#endif

	printf("[170] support #define 0-arg macro func\n");
#define M7() 1
	int M7 = 5;
	assert(1, M7(), "M7()");
	assert(5, M7, "M7");

#define M7 ()
	assert(3, ret3 M7, "ret3 M7");

	printf("[171] support #define multi-arg macro func\n");
#define M8(x, y) x + y
	assert(7, M8(3, 4), "M8(3, 4)");

#define M8(x, y) x *y
	assert(24, M8(3 + 4, 4 + 5), "M8(3+4, 4+5)");

#define M8(x, y) (x) * (y)
	assert(63, M8(3 + 4, 4 + 5), "M8(3+4, 4+5)");

	printf("[172] support empty arg for macro\n");
#define M8(x, y) x y
	assert(9, M8(, 4 + 5), "M8(, 4+5)");

	printf("[172] allow bracketed expressions as macro arguments\n");
#define M8(x, y) x *y
	assert(20, M8((2 + 3), 4), "M8((2+3), 4)");
#define M8(x, y) x *y
	assert(12, M8((2, 3), 4), "M8((2,3), 4)");

	printf("[173] expand only once in a macro function\n");
#define dbl(x) M10(x) * x
#define M10(x) dbl(x) + 3
	assert(10, dbl(2), "dbl(2)");

	printf("[174] support macro characterization operator #\n");
#define M11(x) #x
	// clang-format off
	assert('a', M11(a!b  `""c)[0], "M11( a!b  `\"\"c)[0]");
	assert('!', M11(a!b  `""c)[1], "M11( a!b  `\"\"c)[1]");
	assert('b', M11(a!b  `""c)[2], "M11( a!b  `\"\"c)[2]");
	assert(' ', M11(a!b  `""c)[3], "M11( a!b  `\"\"c)[3]");
	assert('`', M11(a!b  `""c)[4], "M11( a!b  `\"\"c)[4]");
	assert('"', M11(a!b  `""c)[5], "M11( a!b  `\"\"c)[5]");
	assert('"', M11(a!b  `""c)[6], "M11( a!b  `\"\"c)[6]");
	assert('c', M11(a!b  `""c)[7], "M11( a!b  `\"\"c)[7]");
	assert(0, M11(a!b  `""c)[8], "M11( a!b  `\"\"c)[8]");
	// clang-format on

	printf("[175] support macro ## operator\n");
	// clang-format off
#define paste(x, y) x##y
	assert(15, paste(1,5), "paste(1,5)");
	assert(255, paste(0,xff), "paste(0,xff)");
	assert(3, ({ int foobar=3; paste(foo,bar); }), "({ int foobar=3; paste(foo,bar); })");
	assert(5, paste(5,), "paste(5,)");
	assert(5, paste(,5), "paste(,5)");

#define i 5
	assert(101, ({ int i3=100; paste(1+i,3); }), "({ int i3=100; paste(1+i,3); })");
#undef i

#define paste2(x) x##5
	assert(26, paste2(1+2), "paste2(1+2)");

#define paste3(x) 2##x
	assert(23, paste3(1+2), "paste3(1+2)");

#define paste4(x, y, z) x##y##z
	assert(123, paste4(1,2,3), "paste4(1,2,3)");
	// clang-format on

	printf("[177] support defined() macro operator\n");
#define M12
#if defined(M12)
	m = 3;
#else
	m = 4;
#endif
	ASSERT(3, m);

#define M12
#if defined M12
	m = 3;
#else
	m = 4;
#endif
	ASSERT(3, m);

#if defined(M12) - 1
	m = 3;
#else
	m = 4;
#endif
	ASSERT(4, m);

#if defined(NO_SUCH_MACRO)
	m = 3;
#else
	m = 4;
#endif
	ASSERT(4, m);

	printf("[178] replaces the legacy identifier of 0 in constant expressions\n");
#if no_such_symbol == 0
	m = 5;
#else
	m = 6;
#endif
	ASSERT(5, m);

	printf("[179] new lines and spaces are preserved when macros are expanded\n");
	// clang-format off
#define STR(x) #x
#define M12(x) STR(x)
#define M13(x) M12(foo.x)
	ASSERT(0, strcmp(M13(bar), "foo.bar"));

#define M13(x) M12(foo. x)
	ASSERT(0, strcmp(M13(bar), "foo. bar"));

#define M12 foo
#define M13(x) STR(x)
#define M14(x) M13(x.M12)
	ASSERT(0, strcmp(M14(bar), "bar.foo"));

#define M14(x) M13(x. M12)
	ASSERT(0, strcmp(M14(bar), "bar. foo"));
	// clang-format on

	printf("[181] support #include <...>\n");
#include "include3.h"
	ASSERT(3, foo);

#include "include4.h"
	ASSERT(4, foo);

#define M13 "include3.h"
#include M13
	ASSERT(3, foo);

#define M13 < include4.h
#include M13>
	ASSERT(4, foo);

#undef foo

	printf("[186] support __FILE__ and __LINE__\n");
	ASSERT(0, strcmp(main_filename1, "test/macro.c"));
	ASSERT(9, main_line1);
	ASSERT(11, main_line2);
	ASSERT(0, strcmp(include1_filename, "test/include1.h"));
	ASSERT(5, include1_line);

	printf("OK\n");
	return 0;
}
