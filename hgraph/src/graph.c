#include "internal.h"
#include "mem_layout.h"
#include "ptr_table.h"
#include <string.h>

HGRAPH_PRIVATE void
hgraph_find_node_type(
	const hgraph_t* graph,
	const hgraph_node_t* node,
	const hgraph_node_type_info_t** info,
	const hgraph_node_type_t** def
) {
	const hgraph_registry_t* registry = graph->registry;
	const hgraph_node_type_info_t* type_info = &registry->node_types[node->type];
	const hgraph_node_type_t* type_def = type_info->definition;

	*info = type_info;
	*def = type_def;
}

HGRAPH_PRIVATE hgraph_node_t*
hgraph_find_node_by_id(
	const hgraph_t* graph,
	hgraph_index_t node_id
) {
	hgraph_index_t node_slot = hgraph_slot_map_slot_for_id(&graph->node_slot_map, node_id);
	if (!HGRAPH_IS_VALID_INDEX(node_slot)) { return NULL; }

	return (hgraph_node_t*)(graph->nodes + graph->node_size * node_slot);
}

HGRAPH_PRIVATE hgraph_str_t
hgraph_get_node_name_internal(hgraph_t* graph, hgraph_node_t* node) {
	const hgraph_node_type_info_t* type_info;
	const hgraph_node_type_t* type_def;
	hgraph_find_node_type(graph, node, &type_info, &type_def);

	char* name_storage = (char*)node + type_info->size;
	return (hgraph_str_t){
		.data = name_storage,
		.length = node->name_len,
	};
}

HGRAPH_PRIVATE hgraph_index_t
hgraph_encode_pin_id(
	hgraph_index_t node_id,
	hgraph_index_t pin_index,
	bool is_output
) {
	return (node_id << 8) | (pin_index << 1) | (is_output & 0x01);
}

HGRAPH_PRIVATE void
hgraph_decode_pin_id(
	hgraph_index_t pin_id,
	hgraph_index_t* node_id,
	hgraph_index_t* pin_index,
	bool* is_output
) {
	*node_id = pin_id >> 8;
	*pin_index = (pin_id & 0xff) >> 1;
	*is_output = pin_id & 0x01;
}

HGRAPH_PRIVATE hgraph_edge_link_t*
hgraph_resolve_edge(
	hgraph_t* graph,
	hgraph_edge_link_t* pin,
	hgraph_index_t edge_id
) {
	hgraph_index_t slot = hgraph_slot_map_slot_for_id(
		&graph->edge_slot_map,
		edge_id
	);

	return HGRAPH_IS_VALID_INDEX(slot) ? &graph->edges[slot].output_pin_link : pin;
}

hgraph_t*
hgraph_init(
	const hgraph_registry_t* registry,
	const hgraph_config_t* config,
	void* mem,
	size_t* mem_size_inout
) {
	mem_layout_t layout = { 0 };
	mem_layout_reserve(
		&layout,
		sizeof(hgraph_t),
		_Alignof(hgraph_t)
	);

	size_t node_size = registry->max_node_size + config->max_name_length;
	node_size = mem_layout_align_ptr((intptr_t)node_size, _Alignof(max_align_t));
	ptrdiff_t nodes_offset = mem_layout_reserve(
		&layout,
		node_size * config->max_nodes,
		_Alignof(max_align_t)
	);

	ptrdiff_t node_slot_map_offset = hgraph_slot_map_reserve(&layout, config->max_nodes);

	size_t max_edges = config->max_nodes * registry->max_edges_per_node;
	ptrdiff_t edge_slot_map_offset = hgraph_slot_map_reserve(&layout, max_edges);
	ptrdiff_t edges_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_edge_t) * max_edges,
		_Alignof(hgraph_edge_t)
	);

	size_t required_size = mem_layout_size(&layout);
	if (mem == NULL || required_size > *mem_size_inout) {
		*mem_size_inout = required_size;
		return NULL;
	}

	hgraph_t* graph = mem;
	*graph = (hgraph_t){
		.registry = registry,
		.config = *config,
		.node_size = node_size,
		.nodes = mem_layout_locate(graph, nodes_offset),
		.edges = mem_layout_locate(graph, edges_offset),
	};
	hgraph_slot_map_init(
		&graph->node_slot_map,
		config->max_nodes,
		mem_layout_locate(graph, node_slot_map_offset)
	);
	hgraph_slot_map_init(
		&graph->edge_slot_map,
		max_edges,
		mem_layout_locate(graph, edge_slot_map_offset)
	);

	*mem_size_inout = required_size;
	return graph;
}

hgraph_index_t
hgraph_create_node(hgraph_t* graph, const hgraph_node_type_t* type) {
	const hgraph_registry_t* registry = graph->registry;
	const hgraph_node_type_info_t* type_info = hgraph_ptr_table_lookup(&registry->node_type_by_definition, type);
	if (type_info == NULL) { return HGRAPH_INVALID_INDEX; }

	hgraph_index_t node_id, node_slot;
	hgraph_slot_map_allocate(
		&graph->node_slot_map,
		&node_id,
		&node_slot
	);
	if (!HGRAPH_IS_VALID_INDEX(node_id)) { return HGRAPH_INVALID_INDEX; }

	hgraph_node_t* node = (hgraph_node_t*)(graph->nodes + graph->node_size * node_slot);
	// It doesn't matter whether this is initialized.
	// Whenever we allocate a node, incrementing the version will invalidate all
	// previously cached data, if any.
	++node->version;
	node->name_len = 0;
	node->type = type_info - registry->node_types;

	for (hgraph_index_t i = 0; i < type_info->num_attributes; ++i) {
		void* value = (char*)node + type_info->attributes[i].offset;

		if (type->attributes[i]->init != NULL) {
			type->attributes[i]->init(value);
		} else if (type->attributes[i]->data_type->init != NULL) {
			type->attributes[i]->data_type->init(value);
		} else {
			memset(value, 0, type->attributes[i]->data_type->size);
		}
	}

	for (hgraph_index_t i = 0; i < type_info->num_input_pins; ++i) {
		hgraph_index_t* pin = (hgraph_index_t*)((char*)node + type_info->input_pins[i].offset);
		*pin = HGRAPH_INVALID_INDEX;
	}

	for (hgraph_index_t i = 0; i < type_info->num_output_pins; ++i) {
		hgraph_edge_link_t* pin = (hgraph_edge_link_t*)((char*)node + type_info->output_pins[i].offset);
		pin->next = HGRAPH_INVALID_INDEX;
		pin->prev = HGRAPH_INVALID_INDEX;
	}

	return node_id;
}

void
hgraph_destroy_node(hgraph_t* graph, hgraph_index_t id) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, id);
	if (node == NULL) { return; }

	// Destroy edges
	const hgraph_node_type_info_t* type_info;
	const hgraph_node_type_t* type_def;
	hgraph_find_node_type(graph, node, &type_info, &type_def);

	for (hgraph_index_t i = 0; i < type_info->num_input_pins; ++i) {
		hgraph_index_t* input_pin = (hgraph_index_t*)((char*)node + type_info->input_pins[i].offset);
		hgraph_disconnect(graph, *input_pin);
	}

	for (hgraph_index_t i = 0; i < type_info->num_output_pins; ++i) {
		hgraph_edge_link_t* output_pin = (hgraph_edge_link_t*)((char*)node + type_info->output_pins[i].offset);

		while (true) {
			hgraph_edge_link_t* link = hgraph_resolve_edge(graph, output_pin, output_pin->next);
			if (link == output_pin) { break; }

			hgraph_edge_t* edge = HGRAPH_CONTAINER_OF(link, hgraph_edge_t, output_pin_link);
			hgraph_index_t edge_slot = edge - graph->edges;
			hgraph_index_t edge_id = hgraph_slot_map_id_for_slot(
				&graph->edge_slot_map,
				edge_slot
			);
			hgraph_disconnect(graph, edge_id);
		}
	}

	// Destroy node
	hgraph_index_t src_slot, dst_slot;
	hgraph_slot_map_free(&graph->node_slot_map, id, &dst_slot, &src_slot);
	HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(src_slot));

	size_t node_size = graph->node_size;
	char* src_node = graph->nodes + node_size * src_slot;
	char* dst_node = graph->nodes + node_size * dst_slot;
	memcpy(dst_node, src_node, node_size);
}

hgraph_index_t
hgraph_get_pin_id(
	hgraph_t* graph,
	hgraph_index_t node_id,
	const hgraph_pin_description_t* pin
) {
	const hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return HGRAPH_INVALID_INDEX; }

	const hgraph_node_type_info_t* type_info;
	const hgraph_node_type_t* type_def;
	hgraph_find_node_type(graph, node, &type_info, &type_def);

	for (hgraph_index_t i = 0; i < type_info->num_input_pins; ++i) {
		if (type_def->input_pins[i] == pin) {
			return hgraph_encode_pin_id(node_id, i, false);
		}
	}

	for (hgraph_index_t i = 0; i < type_info->num_output_pins; ++i) {
		if (type_def->output_pins[i] == pin) {
			return hgraph_encode_pin_id(node_id, i, true);
		}
	}

	return HGRAPH_INVALID_INDEX;
}

void
hgraph_resolve_pin(
	hgraph_t* graph,
	hgraph_index_t pin_id,
	hgraph_index_t* node_id_out,
	const hgraph_pin_description_t** pin_desc_out
) {
	hgraph_index_t node_id, pin_index;
	bool is_output;
	hgraph_decode_pin_id(pin_id, &node_id, &pin_index, &is_output);

	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) {
		*node_id_out = HGRAPH_INVALID_INDEX;
		*pin_desc_out = NULL;
		return;
	}

	const hgraph_node_type_info_t* type_info;
	const hgraph_node_type_t* type_def;
	hgraph_find_node_type(graph, node, &type_info, &type_def);

	*node_id_out = node_id;

	if (is_output && pin_index < type_info->num_output_pins) {
		*pin_desc_out = type_def->output_pins[pin_index];
	} else if (!is_output && pin_index < type_info->num_input_pins) {
		*pin_desc_out = type_def->input_pins[pin_index];
	} else {
		*pin_desc_out = NULL;
	}
}

hgraph_index_t
hgraph_connect(
	hgraph_t* graph,
	hgraph_index_t from_pin,
	hgraph_index_t to_pin
) {
	if (!(HGRAPH_IS_VALID_INDEX(from_pin) && HGRAPH_IS_VALID_INDEX(to_pin))) {
		return HGRAPH_INVALID_INDEX;
	}

	bool is_output;
	hgraph_index_t from_node_id, from_pin_index;
	hgraph_decode_pin_id(from_pin, &from_node_id, &from_pin_index, &is_output);
	if (!is_output) { return HGRAPH_INVALID_INDEX; }

	hgraph_index_t to_node_id, to_pin_index;
	hgraph_decode_pin_id(to_pin, &to_node_id, &to_pin_index, &is_output);
	if (is_output) { return HGRAPH_INVALID_INDEX; }

	hgraph_node_t* from_node = hgraph_find_node_by_id(graph, from_node_id);
	hgraph_node_t* to_node = hgraph_find_node_by_id(graph, to_node_id);
	if ((from_node == NULL) || (to_node == NULL)) { return HGRAPH_INVALID_INDEX; }

	const hgraph_node_type_info_t* from_type_info;
	const hgraph_node_type_t* from_type_def;
	hgraph_find_node_type(graph, from_node, &from_type_info, &from_type_def);
	if (from_pin_index >= from_type_info->num_output_pins) { return HGRAPH_INVALID_INDEX; }

	const hgraph_node_type_info_t* to_type_info;
	const hgraph_node_type_t* to_type_def;
	hgraph_find_node_type(graph, to_node, &to_type_info, &to_type_def);
	if (to_pin_index >= to_type_info->num_input_pins) { return HGRAPH_INVALID_INDEX; }

	const hgraph_data_type_t* from_data_type = from_type_def->output_pins[from_pin_index]->data_type;
	const hgraph_data_type_t* to_data_type = to_type_def->input_pins[to_pin_index]->data_type;
	if (from_data_type != to_data_type) { return HGRAPH_INVALID_INDEX; }

	hgraph_index_t* input_pin = (hgraph_index_t*)((char*)to_node + to_type_info->input_pins[to_pin_index].offset);
	if (HGRAPH_IS_VALID_INDEX(*input_pin)) { return HGRAPH_INVALID_INDEX; }

	hgraph_edge_link_t* output_pin = (hgraph_edge_link_t*)((char*)from_node + from_type_info->output_pins[from_pin_index].offset);

	// Create edge
	hgraph_index_t edge_id, edge_slot;
	hgraph_slot_map_allocate(
		&graph->edge_slot_map,
		&edge_id,
		&edge_slot
	);
	if (!HGRAPH_IS_VALID_INDEX(edge_id)) { return HGRAPH_INVALID_INDEX; }
	hgraph_edge_t* edge = &graph->edges[edge_slot];
	edge->from_pin = from_pin;
	edge->to_pin = to_pin;

	// Connect input pin
	*input_pin = edge_id;

	// Connect output pin
	edge->output_pin_link.prev = output_pin->prev;
	edge->output_pin_link.next = HGRAPH_INVALID_INDEX;
	hgraph_edge_link_t* prev = hgraph_resolve_edge(graph, output_pin, output_pin->prev);
	prev->next = edge_id;
	output_pin->prev = edge_id;

	return edge_id;
}

void
hgraph_disconnect(hgraph_t* graph, hgraph_index_t edge_id) {
	hgraph_index_t edge_slot = hgraph_slot_map_slot_for_id(
		&graph->edge_slot_map,
		edge_id
	);
	if (!HGRAPH_IS_VALID_INDEX(edge_id)) { return; }

	hgraph_edge_t* edge = &graph->edges[edge_slot];
	bool is_output;
	hgraph_index_t from_node_id, from_pin_index, to_node_id, to_pin_index;
	hgraph_decode_pin_id(edge->from_pin, &from_node_id, &from_pin_index, &is_output);
	hgraph_decode_pin_id(edge->to_pin, &to_node_id, &to_pin_index, &is_output);

	hgraph_node_t* from_node = hgraph_find_node_by_id(graph, from_node_id);
	hgraph_node_t* to_node = hgraph_find_node_by_id(graph, to_node_id);
	HGRAPH_ASSERT((from_node != NULL) && (to_node != NULL));

	// Disconnect input pin
	const hgraph_node_type_info_t* to_type_info;
	const hgraph_node_type_t* to_type_def;
	hgraph_find_node_type(graph, to_node, &to_type_info, &to_type_def);
	hgraph_index_t* input_pin = (hgraph_index_t*)((char*)to_node + to_type_info->input_pins[to_pin_index].offset);
	HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(*input_pin));
	*input_pin = HGRAPH_INVALID_INDEX;

	// Disconnect output pin
	const hgraph_node_type_info_t* from_type_info;
	const hgraph_node_type_t* from_type_def;
	hgraph_find_node_type(graph, from_node, &from_type_info, &from_type_def);
	hgraph_edge_link_t* output_pin = (hgraph_edge_link_t*)((char*)from_node + from_type_info->output_pins[from_pin_index].offset);

	hgraph_edge_link_t* prev = hgraph_resolve_edge(graph, output_pin, edge->output_pin_link.prev);
	hgraph_edge_link_t* next = hgraph_resolve_edge(graph, output_pin, edge->output_pin_link.next);
	prev->next = edge->output_pin_link.next;
	next->prev = edge->output_pin_link.prev;

	// Destroy edge
	hgraph_index_t dst_slot, src_slot;
	hgraph_slot_map_free(
		&graph->edge_slot_map,
		edge_id,
		&dst_slot,
		&src_slot
	);
	HGRAPH_ASSERT(
		HGRAPH_IS_VALID_INDEX(dst_slot) && HGRAPH_IS_VALID_INDEX(src_slot)
	);
	graph->edges[dst_slot] = graph->edges[src_slot];
}

hgraph_str_t
hgraph_get_node_name(hgraph_t* graph, hgraph_index_t node_id) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return (hgraph_str_t){ 0 }; }

	return hgraph_get_node_name_internal(graph, node);
}

void
hgraph_set_node_name(hgraph_t* graph, hgraph_index_t node_id, hgraph_str_t name) {
	if (name.length > graph->config.max_name_length) { return; }

	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return; }

	const hgraph_node_type_info_t* type_info;
	const hgraph_node_type_t* type_def;
	hgraph_find_node_type(graph, node, &type_info, &type_def);

	char* name_storage = (char*)node + type_info->size;
	memcpy(name_storage, name.data, name.length);
	node->name_len = name.length;
}

hgraph_index_t
hgraph_get_node_by_name(hgraph_t* graph, hgraph_str_t name) {
	for (hgraph_index_t i = 0; i < graph->node_slot_map.num_items; ++i) {
		hgraph_node_t* node = (hgraph_node_t*)(graph->nodes + graph->node_size * i);
		hgraph_str_t node_name = hgraph_get_node_name_internal(graph, node);

		if (hgraph_str_equal(name, node_name)) {
			return hgraph_slot_map_id_for_slot(&graph->node_slot_map, i);
		}
	}

	return HGRAPH_INVALID_INDEX;
}

void
hgraph_set_node_attribute(
	hgraph_t* graph,
	hgraph_index_t node_id,
	const hgraph_attribute_description_t* attribute,
	const void* value
) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return; }

	const hgraph_node_type_info_t* type_info;
	const hgraph_node_type_t* type_def;
	hgraph_find_node_type(graph, node, &type_info, &type_def);

	for (hgraph_index_t i = 0; i < type_info->num_attributes; ++i) {
		if (type_def->attributes[i] == attribute) {
			char* storage = (char*)node + type_info->attributes[i].offset;
			if (attribute->data_type->assign != NULL) {
				attribute->data_type->assign(storage, value);
			} else {
				memcpy(storage, value, attribute->data_type->size);
			}

			break;
		}
	}
}

const void*
hgraph_get_node_attribute(
	hgraph_t* graph,
	hgraph_index_t node_id,
	const hgraph_attribute_description_t* attribute
) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return NULL; }

	const hgraph_node_type_info_t* type_info;
	const hgraph_node_type_t* type_def;
	hgraph_find_node_type(graph, node, &type_info, &type_def);

	for (hgraph_index_t i = 0; i < type_info->num_attributes; ++i) {
		if (type_def->attributes[i] == attribute) {
			return (char*)node + type_info->attributes[i].offset;
		}
	}

	return NULL;
}

void
hgraph_iterate_nodes(
	hgraph_t* graph,
	hgraph_node_iterator_t iterator,
	void* userdata
) {
	for (hgraph_index_t i = 0; i < graph->node_slot_map.num_items; ++i) {
		const hgraph_node_t* node = (hgraph_node_t*)(graph->nodes + graph->node_size * i);
		hgraph_index_t id = hgraph_slot_map_id_for_slot(&graph->node_slot_map, i);

		// May happen if removal occurs during iteration
		if (!HGRAPH_IS_VALID_INDEX(id)) { continue; }

		const hgraph_node_type_t* type = graph->registry->node_types[node->type].definition;
		if (!iterator(id, type, userdata)) { break; }
	}
}

void
hgraph_iterate_edges(
	hgraph_t* graph,
	hgraph_edge_iterator_t iterator,
	void* userdata
) {
	for (hgraph_index_t i = 0; i < graph->edge_slot_map.num_items; ++i) {
		const hgraph_edge_t* edge = &graph->edges[i];
		hgraph_index_t id = hgraph_slot_map_id_for_slot(&graph->edge_slot_map, i);

		// May happen if removal occurs during iteration
		if (!HGRAPH_IS_VALID_INDEX(id)) { continue; }

		if (!iterator(id, edge->from_pin, edge->to_pin, userdata)) { break; }
	}
}

void
hgraph_iterate_edges_to(
	hgraph_t* graph,
	hgraph_index_t node_id,
	hgraph_edge_iterator_t iterator,
	void* userdata
) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return; }

	const hgraph_node_type_info_t* type_info;
	const hgraph_node_type_t* type_def;
	hgraph_find_node_type(graph, node, &type_info, &type_def);

	for (hgraph_index_t i = 0; i < type_info->num_input_pins; ++i) {
		hgraph_index_t* input_pin = (hgraph_index_t*)((char*)node + type_info->input_pins[i].offset);
		hgraph_index_t edge_id = *input_pin;
		hgraph_index_t edge_slot = hgraph_slot_map_slot_for_id(
			&graph->edge_slot_map,
			edge_id
		);
		if (!HGRAPH_IS_VALID_INDEX(edge_slot)) { continue; }

		hgraph_edge_t* edge = &graph->edges[edge_slot];

		if (!iterator(edge_id, edge->from_pin, edge->to_pin, userdata)) {
			break;
		}
	}
}

void
hgraph_iterate_edges_from(
	hgraph_t* graph,
	hgraph_index_t node_id,
	hgraph_edge_iterator_t iterator,
	void* userdata
) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return; }

	const hgraph_node_type_info_t* type_info;
	const hgraph_node_type_t* type_def;
	hgraph_find_node_type(graph, node, &type_info, &type_def);

	for (hgraph_index_t i = 0; i < type_info->num_output_pins; ++i) {
		hgraph_edge_link_t* output_pin = (hgraph_edge_link_t*)((char*)node + type_info->output_pins[i].offset);

		hgraph_index_t itr = output_pin->next;
		while (true) {
			hgraph_edge_link_t* link = hgraph_resolve_edge(graph, output_pin, itr);
			if (link == output_pin) { break; }

			hgraph_edge_t* edge = HGRAPH_CONTAINER_OF(link, hgraph_edge_t, output_pin_link);
			hgraph_index_t edge_slot = edge - graph->edges;
			hgraph_index_t edge_id = hgraph_slot_map_id_for_slot(
				&graph->edge_slot_map,
				edge_slot
			);
			itr = link->next;

			if (!iterator(edge_id, edge->from_pin, edge->to_pin, userdata)) {
				break;
			}
		}
	}
}
