#include "slot_map.h"
#include "mem_layout.h"

ptrdiff_t
hgraph_slot_map_reserve(mem_layout_t* layout, hgraph_index_t max_items) {
	return mem_layout_reserve(
		layout,
		sizeof(hgraph_index_t) * max_items * 2,
		_Alignof(hgraph_index_t)
	);
}

void
hgraph_slot_map_init(
	hgraph_slot_map_t* slot_map,
	hgraph_index_t max_items,
	void* memory
) {
	slot_map->max_items = max_items;
	slot_map->num_items = 0;
	slot_map->slots_for_id = memory;
	slot_map->ids_for_slot = slot_map->slots_for_id + max_items;

	for (hgraph_index_t i = 0; i < max_items; ++i) {
		slot_map->slots_for_id[i] = slot_map->ids_for_slot[i] = i;
	}
}

void
hgraph_slot_map_allocate(
	hgraph_slot_map_t* slot_map,
	hgraph_index_t* id_out,
	hgraph_index_t* slot_index_out
) {
	if (slot_map->num_items == slot_map->max_items) {
		*id_out = HGRAPH_INVALID_INDEX;
		*slot_index_out = HGRAPH_INVALID_INDEX;
		return;
	}

	hgraph_index_t slot = ++slot_map->num_items;
	*id_out = slot_map->ids_for_slot[slot];
	*slot_index_out = slot;
}

void
hgraph_slot_map_free(
	hgraph_slot_map_t* slot_map,
	hgraph_index_t id,
	hgraph_index_t* dst_slot_index_out,
	hgraph_index_t* src_slot_index_out
) {
	hgraph_index_t deleted_slot = hgraph_slot_map_slot_for_id(slot_map, id);
	if (!HGRAPH_IS_VALID_INDEX(deleted_slot)) {
		*dst_slot_index_out = HGRAPH_INVALID_INDEX;
		*src_slot_index_out = HGRAPH_INVALID_INDEX;
		return;
	}

	hgraph_index_t last_slot = --slot_map->num_items;
	hgraph_index_t last_item_id = slot_map->ids_for_slot[last_slot];

	slot_map->ids_for_slot[last_slot] = id;
	slot_map->ids_for_slot[deleted_slot] = last_item_id;
	slot_map->slots_for_id[last_item_id] = deleted_slot;
	slot_map->slots_for_id[id] = last_slot;

	*dst_slot_index_out = deleted_slot;
	*src_slot_index_out = last_slot;
}

hgraph_index_t
hgraph_slot_map_slot_for_id(const hgraph_slot_map_t* slot_map, hgraph_index_t id) {
	if (!((0 <= id) && (id < slot_map->max_items))) { return HGRAPH_INVALID_INDEX; }

	hgraph_index_t slot = slot_map->slots_for_id[id];
	return ((0 <= slot) && (slot < slot_map->num_items)) ? slot : HGRAPH_INVALID_INDEX;
}

hgraph_index_t
hgraph_slot_map_id_for_slot(const hgraph_slot_map_t* slot_map, hgraph_index_t slot) {
	return ((0 <= slot) && (slot < slot_map->num_items))
		? slot_map->ids_for_slot[slot]
		: HGRAPH_INVALID_INDEX;
}
