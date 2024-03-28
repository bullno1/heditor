#ifndef HGRAPH_PTR_TABLE_H
#define HGRAPH_PTR_TABLE_H

#include <hgraph/common.h>
#include "mem_layout.h"

typedef struct hgraph_ptr_pair_s {
	const void* key;
	void* value;
} hgraph_ptr_pair_t;

typedef struct hgraph_ptr_table_s {
	hgraph_index_t exp;
	hgraph_ptr_pair_t* entries;
} hgraph_ptr_table_t;

ptrdiff_t
hgraph_ptr_table_reserve(mem_layout_t* layout, hgraph_index_t num_entries);

void
hgraph_ptr_table_init(
	hgraph_ptr_table_t* table,
	hgraph_index_t num_entries,
	void* memory
);

void
hgraph_ptr_table_put(
	hgraph_ptr_table_t* table,
	const void* key,
	void* value
);

void*
hgraph_ptr_table_lookup(
	hgraph_ptr_table_t* table,
	const void* key
);

#endif
