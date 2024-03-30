#ifndef HGRAPH_SLIP_H
#define HGRAPH_SLIP_H

// Implement https://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol

#include <hgraph/io.h>

typedef struct hgraph_slip_out_s {
	hgraph_out_t impl;
	hgraph_out_t* out;
} hgraph_slip_out_t;

typedef struct hgraph_slip_in_s {
	hgraph_in_t impl;
	hgraph_in_t* in;
} hgraph_slip_in_t;

hgraph_out_t*
hgraph_slip_out_init(hgraph_slip_out_t* slip, hgraph_out_t* out);

hgraph_io_status_t
hgraph_slip_out_end(hgraph_out_t* out);

hgraph_in_t*
hgraph_slip_in_init(hgraph_slip_in_t* slip, hgraph_in_t* in);

hgraph_io_status_t
hgraph_slip_in_skip(hgraph_in_t* in);

hgraph_io_status_t
hgraph_slip_in_end(hgraph_in_t* in);

#endif
