#include "slip.h"
#include "internal.h"

static const char HGRAPH_SLIP_END = 0xc0;
static const char HGRAPH_SLIP_ESC = 0xdb;
static const char HGRAPH_SLIP_ESC_END = 0xdc;
static const char HGRAPH_SLIP_ESC_ESC = 0xdd;
static const char HGRAPH_SLIP_ESCAPED_END[] = { HGRAPH_SLIP_ESC, HGRAPH_SLIP_ESC_END };
static const char HGRAPH_SLIP_ESCAPED_ESC[] = { HGRAPH_SLIP_ESC, HGRAPH_SLIP_ESC_ESC };

HGRAPH_PRIVATE size_t
hgraph_slip_out_write(hgraph_out_t* impl, const void* buf, size_t size) {
	hgraph_slip_out_t* slip = HGRAPH_CONTAINER_OF(impl, hgraph_slip_out_t, impl);
	hgraph_out_t* out = slip->out;
	const char* chars = buf;

	for (size_t i = 0; i < size; ++i) {
		char ch = chars[i];

		const char* escaped_buf;
		size_t escaped_size;
		if (ch == HGRAPH_SLIP_END) {
			escaped_buf = HGRAPH_SLIP_ESCAPED_END;
			escaped_size = sizeof(HGRAPH_SLIP_ESC_END);
		} else if (ch == HGRAPH_SLIP_ESC) {
			escaped_buf = HGRAPH_SLIP_ESCAPED_ESC;
			escaped_size = sizeof(HGRAPH_SLIP_ESCAPED_ESC);
		} else {
			escaped_buf = &ch;
			escaped_size = sizeof(ch);
		}

		if (hgraph_io_write(out, escaped_buf, escaped_size) != HGRAPH_IO_OK) {
			return i;
		}
	}

	return size;
}

HGRAPH_PRIVATE size_t
hgraph_slip_in_read(hgraph_in_t* impl, void* buf, size_t size) {
	hgraph_slip_in_t* slip = HGRAPH_CONTAINER_OF(impl, hgraph_slip_in_t, impl);
	hgraph_in_t* in = slip->in;
	char* chars = buf;

	for (size_t i = 0; i < size; ++i) {
		char ch;
		if (hgraph_io_read(in, &ch, sizeof(ch)) != HGRAPH_IO_OK) { return i; }

		if (ch == HGRAPH_SLIP_ESC) {
			if (hgraph_io_read(in, &ch, sizeof(ch)) != HGRAPH_IO_OK) { return i; }

			if (ch == HGRAPH_SLIP_ESC_END) {
				chars[i] = HGRAPH_SLIP_END;
			} else if (ch == HGRAPH_SLIP_ESC_ESC) {
				chars[i] = HGRAPH_SLIP_ESC;
			} else {
				return i;
			}
		} else if (ch == HGRAPH_SLIP_END) {
			return i;
		} else {
			chars[i] = ch;
		}
	}

	return size;
}

hgraph_out_t*
hgraph_slip_out_init(hgraph_slip_out_t* slip, hgraph_out_t* out) {
	slip->impl.write = hgraph_slip_out_write;
	slip->out = out;
	return &slip->impl;
}

hgraph_io_status_t
hgraph_slip_out_end(hgraph_out_t* out) {
	return hgraph_io_write(out, &HGRAPH_SLIP_END, sizeof(HGRAPH_SLIP_END));
}

hgraph_in_t*
hgraph_slip_in_init(hgraph_slip_in_t* slip, hgraph_in_t* in) {
	slip->impl.read = hgraph_slip_in_read;
	slip->in = in;
	return &slip->impl;
}

hgraph_io_status_t
hgraph_slip_in_skip(hgraph_in_t* in) {
	while (true) {
		char ch;
		HGRAPH_CHECK_IO(hgraph_io_read(in, &ch, sizeof(ch)));

		if (ch == HGRAPH_SLIP_END) {
			return HGRAPH_IO_OK;
		}
	}
}

hgraph_io_status_t
hgraph_slip_in_end(hgraph_in_t* in) {
	char ch;
	HGRAPH_CHECK_IO(hgraph_io_read(in, &ch, sizeof(ch)));

	return ch == HGRAPH_SLIP_END ? HGRAPH_IO_OK : HGRAPH_IO_MALFORMED;
}
