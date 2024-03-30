#ifndef HGRAPH_COMMON_H
#define HGRAPH_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HGRAPH_STR(STR) (hgraph_str_t){ .length = sizeof(STR) - 1, .data = STR }
#define HGRAPH_INVALID_INDEX ((hgraph_index_t)-1)
#define HGRAPH_IS_VALID_INDEX(X) ((X) >= 0)
#if __STDC_VERSION__ >= 201112L
#	define HGRAPH_STATIC_ASSERT(X, MSG) _Static_assert(X, MSG)
#else
#	include <assert.h>
#	define HGRAPH_STATIC_ASSERT(X, MSG) assert((X) && (MSG))
#endif

typedef int32_t hgraph_index_t;

typedef struct hgraph_str_s {
	hgraph_index_t length;
	const char* data;
} hgraph_str_t;

typedef struct hgraph_in_s {
	size_t (*read)(
		struct hgraph_in_s* input,
		void* buffer,
		size_t size
	);
} hgraph_in_t;

typedef struct hgraph_out_s {
	size_t (*write)(
		struct hgraph_out_s* output,
		const void* buffer,
		size_t size
	);
} hgraph_out_t;

typedef enum hgraph_io_status_e {
	HGRAPH_IO_OK,
	HGRAPH_IO_ERROR,
	HGRAPH_IO_MALFORMED,
} hgraph_io_status_t;

#endif
