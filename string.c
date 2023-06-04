#include "thrvcc.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

// return formated string
char *format(char *fmt, ...)
{
	char *buf;
	size_t buf_len;
	// make string-memory I/O stream
	FILE *out = open_memstream(&buf, &buf_len);

	va_list va;
	va_start(va, fmt);
	// write data in stream
	vfprintf(out, fmt, va);
	va_end(va);

	fclose(out);
	return buf;
}
