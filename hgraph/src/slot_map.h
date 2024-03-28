#ifndef HGRAPH_SLOT_MAP_H
#define HGRAPH_SLOT_MAP_H

#include <hgraph/common.h>
#include "mem_layout.h"

typedef struct hgraph_slot_map_s {
	hgraph_index_t max_items;
	hgraph_index_t num_items;
	hgraph_index_t* ids_for_slot;
	hgraph_index_t* slots_for_id;
} hgraph_slot_map_t;

ptrdiff_t
hgraph_slot_map_reserve(mem_layout_t* layout, hgraph_index_t max_items);

void
hgraph_slot_map_init(
	hgraph_slot_map_t* slot_map,
	hgraph_index_t max_items,
	void* memory
);

void
hgraph_slot_map_allocate(
	hgraph_slot_map_t* slot_map,
	hgraph_index_t* id_out,
	hgraph_index_t* slot_index_out
);

void
hgraph_slot_map_free(
	hgraph_slot_map_t* slot_map,
	hgraph_index_t id,
	hgraph_index_t* dst_slot_index_out,
	hgraph_index_t* src_slot_index_out
);

hgraph_index_t
hgraph_slot_map_slot_for_id(const hgraph_slot_map_t* slot_map, hgraph_index_t id);

hgraph_index_t
hgraph_slot_map_id_for_slot(const hgraph_slot_map_t* slot_map, hgraph_index_t slot);

#endif
