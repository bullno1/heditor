#ifndef HED_STRING_H
#define HED_STRING_H

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#define HED_STR_LIT(X) (hed_str_t){ .length = sizeof(X) - 1, .data = X }

struct hed_allocator_s;

typedef struct hed_str_s {
	size_t length;
	const char* data;
} hed_str_t;

hed_str_t
hed_strcpy(struct hed_allocator_s* alloc, hed_str_t str);

hed_str_t
hed_str_vformat(struct hed_allocator_s* alloc, const char* fmt, va_list args);

hed_str_t
hed_str_from_cstr(const char* str);

bool
hed_str_equal(hed_str_t lhs, hed_str_t rhs);

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format (printf, 2, 3)))
#endif
static inline hed_str_t
hed_str_format(struct hed_allocator_s* alloc, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	hed_str_t str = hed_str_vformat(alloc, fmt, args);
	va_end(args);

	return str;
}

#endif
