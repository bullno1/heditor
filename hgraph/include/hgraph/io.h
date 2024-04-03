#ifndef HGRAPH_IO_H
#define HGRAPH_IO_H

#include "common.h"
#include <string.h>

#define HGRAPH_CHECK_IO(OP) \
	do { \
		hgraph_io_status_t status = OP; \
		if (status != HGRAPH_IO_OK) { return status; } \
	} while(0)

static inline hgraph_io_status_t
hgraph_io_read(hgraph_in_t* input, void* buffer, size_t size) {
	return input->read(input, buffer, size) == size ? HGRAPH_IO_OK : HGRAPH_IO_ERROR;
}

static inline hgraph_io_status_t
hgraph_io_write(hgraph_out_t* output, const void* buffer, size_t size) {
	return output->write(output, buffer, size) == size ? HGRAPH_IO_OK : HGRAPH_IO_ERROR;
}

static inline hgraph_io_status_t
hgraph_io_write_uint(uint64_t x, hgraph_out_t* out) {
    char buf[10];
    size_t n = 0;

	for (int i = 0; i < 10; ++i) {
		n += x >= 0x80;
		buf[i] = x | 0x80;
		x >>= 7;
	}

    buf[n] ^= 0x80;
	n += 1;

	return hgraph_io_write(out, buf, n);
}

static inline hgraph_io_status_t
hgraph_io_write_sint(int64_t x, hgraph_out_t* out) {
    uint64_t ux = (uint64_t)x << 1;
    if (x < 0) { ux = ~ux; }
    return hgraph_io_write_uint(ux, out);
}

static inline hgraph_io_status_t
hgraph_io_read_uint(uint64_t* x, hgraph_in_t* in) {
	uint64_t b;
	char c;
	uint64_t tmp = 0;

	for (int i = 0; i < 10; ++i) {
		HGRAPH_CHECK_IO(hgraph_io_read(in, &c, 1));

		b = c;
		tmp |= (b & 0x7f) << (7 * i);
		if (b < 0x80) {
			*x = tmp;
			return HGRAPH_IO_OK;
		}
	}

	return HGRAPH_IO_MALFORMED;
}

static inline hgraph_io_status_t
hgraph_io_read_sint(int64_t* x, hgraph_in_t* in) {
    uint64_t ux;

	HGRAPH_CHECK_IO(hgraph_io_read_uint(&ux, in));

    int64_t tmp = (int64_t)(ux >> 1);
    if ((ux & 1) != 0) {
		tmp = ~tmp;
    }

	*x = tmp;
    return HGRAPH_IO_OK;
}

static inline hgraph_io_status_t
hgraph_io_write_f32(float f32, hgraph_out_t* out) {
	uint32_t ivalue;
	HGRAPH_STATIC_ASSERT(sizeof(f32) == sizeof(ivalue), "float is not 32 bit");
	memcpy(&ivalue, &f32, sizeof(f32));

	uint8_t buf[sizeof(ivalue)];
	for (size_t i = 0; i < sizeof(ivalue); ++i) {
		buf[i] = ivalue >> (i * 8);
	}

	return hgraph_io_write(out, buf, sizeof(buf));
}

static inline hgraph_io_status_t
hgraph_io_read_f32(float* f32, hgraph_in_t* in) {
	uint32_t ivalue = 0;
	HGRAPH_STATIC_ASSERT(sizeof(*f32) == sizeof(ivalue), "float is not 32 bit");

	uint8_t buf[sizeof(ivalue)];
	HGRAPH_CHECK_IO(hgraph_io_read(in, buf, sizeof(buf)));
	for (size_t i = 0; i < sizeof(ivalue); ++i) {
		ivalue |= (uint32_t)buf[i] << (i * 8);
	}
	memcpy(f32, &ivalue, sizeof(ivalue));

	return HGRAPH_IO_OK;
}

static inline hgraph_io_status_t
hgraph_io_write_f64(double f64, hgraph_out_t* out) {
	uint64_t ivalue;
	HGRAPH_STATIC_ASSERT(sizeof(f64) == sizeof(ivalue), "double is not 64 bit");
	memcpy(&ivalue, &f64, sizeof(f64));

	uint8_t buf[sizeof(ivalue)];
	for (size_t i = 0; i < sizeof(ivalue); ++i) {
		buf[i] = ivalue >> (i * 8);
	}

	return hgraph_io_write(out, buf, sizeof(buf));
}

static inline hgraph_io_status_t
hgraph_io_read_f64(double* f64, hgraph_in_t* in) {
	uint64_t ivalue = 0;
	HGRAPH_STATIC_ASSERT(sizeof(*f64) == sizeof(ivalue), "float is not 32 bit");

	uint8_t buf[sizeof(ivalue)];
	HGRAPH_CHECK_IO(hgraph_io_read(in, buf, sizeof(buf)));
	for (size_t i = 0; i < sizeof(ivalue); ++i) {
		ivalue |= (uint64_t)buf[i] << (i * 8);
	}
	memcpy(f64, &ivalue, sizeof(ivalue));

	return HGRAPH_IO_OK;
}

static inline hgraph_io_status_t
hgraph_io_write_str(hgraph_str_t str, hgraph_out_t* out) {
	HGRAPH_CHECK_IO(hgraph_io_write_uint((uint64_t)str.length, out));
	HGRAPH_CHECK_IO(hgraph_io_write(out, str.data, (size_t)str.length));

	return HGRAPH_IO_OK;
}

static inline hgraph_io_status_t
hgraph_io_read_str(
	char* buf,
	size_t* len_inout,
	hgraph_in_t* in
) {
	uint64_t length;

	HGRAPH_CHECK_IO(hgraph_io_read_uint(&length, in));
	if (length > *len_inout) { return HGRAPH_IO_MALFORMED; }
	if (length > 0) { HGRAPH_CHECK_IO(hgraph_io_read(in, buf, length)); }

	*len_inout = length;
	return HGRAPH_IO_OK;
}

#endif
