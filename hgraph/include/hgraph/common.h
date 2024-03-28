#ifndef HGRAPH_COMMON_H
#define HGRAPH_COMMON_H

#include <stdint.h>
#include <stddef.h>

#define HGRAPH_STR(STR) (hgraph_str_t){ .size = sizeof(STR) - 1, .data = STR }
#define HGRAPH_INVALID_INDEX ((hgraph_index_t)-1)
#define HGRAPH_IS_VALID_INDEX(X) ((X) >= 0)

typedef int32_t hgraph_index_t;

typedef struct hgraph_str_s {
	hgraph_index_t length;
	const char* data;
} hgraph_str_t;

typedef struct hgraph_out_s {
	hgraph_index_t (*write)(
		struct hgraph_out_s* output,
		const void* buffer,
		hgraph_index_t size
	);
} hgraph_out_t;

typedef struct hgraph_in_s {
	hgraph_index_t (*read)(
		struct hgraph_in_s* input,
		void* buffer,
		hgraph_index_t size
	);
} hgraph_in_t;

static inline hgraph_index_t
hgraph_io_read(
	hgraph_in_t* input,
	void* buffer,
	hgraph_index_t size
) {
	return input->read(input, buffer, size);
}

static inline hgraph_index_t
hgraph_io_write(
	hgraph_out_t* output,
	const void* buffer,
	hgraph_index_t size
) {
	return output->write(output, buffer, size);
}

#endif
