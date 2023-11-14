#include "thrvcc.h"
#include <alloca.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// push string into string array
void str_array_push(struct StringArray *arr, char *s)
{
	// if empty
	if (!arr->data) {
		// alloc 8
		arr->data = calloc(8, sizeof(char *));
		arr->capacity = 8;
	}

	// if full, alloc x2
	if (arr->capacity == arr->len) {
		arr->data =
			realloc(arr->data, sizeof(char *) * arr->capacity * 2);
		arr->capacity *= 2;
		// clean new space
		for (int i = arr->len; i < arr->capacity; i++)
			arr->data[i] = NULL;
	}

	// store
	arr->data[arr->len++] = s;
}

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
