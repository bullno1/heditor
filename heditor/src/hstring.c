#include "hstring.h"
#include "allocator/allocator.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

hed_str_t
hed_strcpy(struct hed_allocator_s* alloc, hed_str_t str) {
	char* data = hed_malloc(str.length + 1, alloc);
	memcpy(data, str.data, str.length);
	data[str.length] = '\0';
	return (hed_str_t){
		.length = str.length,
		.data = data,
	};
}

hed_str_t
hed_str_vformat(struct hed_allocator_s* alloc, const char* fmt, va_list args) {
	va_list args_copy;
	va_copy(args_copy, args);
	int length = vsnprintf(NULL, 0, fmt, args_copy);
	if (length < 0) { return (hed_str_t) { 0 }; }

	char* data = hed_malloc((size_t)length + 1, alloc);
	vsnprintf(data, length + 1, fmt, args);
	data[length] = '\0';
	return (hed_str_t){
		.data = data,
		.length = (size_t)length,
	};
}

hed_str_t
hed_str_from_cstr(const char* str) {
	return (hed_str_t){
		.length = strlen(str),
		.data = str,
	};
}
