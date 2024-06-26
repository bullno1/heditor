#include "internal.h"
#include "mem_layout.h"
#include "ptr_table.h"
#include <string.h>

HGRAPH_INTERNAL const hgraph_node_type_info_t*
hgraph_get_node_type_internal(
	const hgraph_t* graph, const hgraph_node_t* node
) {
	const hgraph_registry_t* registry = graph->registry;
	return &registry->node_types[node->type];
}

HGRAPH_INTERNAL hgraph_node_t*
hgraph_find_node_by_id(
	const hgraph_t* graph,
	hgraph_index_t node_id
) {
	hgraph_index_t node_slot = hgraph_slot_map_slot_for_id(&graph->node_slot_map, node_id);
	if (!HGRAPH_IS_VALID_INDEX(node_slot)) { return NULL; }

	return (hgraph_node_t*)(graph->nodes + graph->node_size * node_slot);
}

HGRAPH_INTERNAL hgraph_node_t*
hgraph_get_node_by_slot(const hgraph_t* graph, hgraph_index_t slot) {
	return (hgraph_node_t*)(graph->nodes + graph->node_size * slot);
}

HGRAPH_INTERNAL hgraph_str_t
hgraph_get_node_name_internal(const hgraph_t* graph, const hgraph_node_t* node) {
	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(graph, node);

	char* name_storage = (char*)node + type_info->size;
	return (hgraph_str_t){
		.data = name_storage,
		.length = node->name_len,
	};
}

HGRAPH_INTERNAL hgraph_index_t
hgraph_encode_pin_id(
	hgraph_index_t node_id,
	hgraph_index_t pin_index,
	bool is_output
) {
	return (node_id << 8) | (pin_index << 1) | (is_output & 0x01);
}

HGRAPH_INTERNAL void
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

HGRAPH_INTERNAL hgraph_edge_link_t*
hgraph_resolve_edge(
	const hgraph_t* graph,
	hgraph_edge_link_t* pin,
	hgraph_index_t edge_id
) {
	hgraph_index_t slot = hgraph_slot_map_slot_for_id(
		&graph->edge_slot_map,
		edge_id
	);

	return HGRAPH_IS_VALID_INDEX(slot) ? &graph->edges[slot].output_pin_link : pin;
}

size_t
hgraph_init(hgraph_t* graph, size_t size, const hgraph_config_t* config) {
	mem_layout_t layout = { 0 };
	mem_layout_reserve(
		&layout,
		sizeof(hgraph_t),
		_Alignof(hgraph_t)
	);

	const hgraph_registry_t* registry = config->registry;
	size_t node_size = registry->max_node_size + config->max_name_length + 1;
	node_size = mem_layout_align_ptr((intptr_t)node_size, _Alignof(max_align_t));
	ptrdiff_t nodes_offset = mem_layout_reserve(
		&layout,
		node_size * config->max_nodes,
		_Alignof(max_align_t)
	);

	ptrdiff_t node_slot_map_offset = hgraph_slot_map_reserve(&layout, config->max_nodes);
	ptrdiff_t node_versions_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_index_t) * config->max_nodes,
		_Alignof(max_align_t)
	);

	size_t max_edges = config->max_nodes * registry->max_edges_per_node;
	ptrdiff_t edge_slot_map_offset = hgraph_slot_map_reserve(&layout, max_edges);
	ptrdiff_t edges_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_edge_t) * max_edges,
		_Alignof(hgraph_edge_t)
	);

	size_t required_size = mem_layout_size(&layout);
	if (graph == NULL || size < required_size) { return required_size; }

	*graph = (hgraph_t){
		.registry = registry,
		.max_name_length = config->max_name_length,
		.node_size = node_size,
		.node_versions = mem_layout_locate(graph, node_versions_offset),
		.nodes = mem_layout_locate(graph, nodes_offset),
		.edges = mem_layout_locate(graph, edges_offset),
	};
	memset(graph->node_versions, 0, sizeof(hgraph_index_t) * config->max_nodes);
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

	return required_size;
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

	hgraph_node_t* node = hgraph_get_node_by_slot(graph, node_slot);
	++graph->node_versions[node_id];
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

	++graph->version;
	return node_id;
}

void
hgraph_destroy_node(hgraph_t* graph, hgraph_index_t id) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, id);
	if (node == NULL) { return; }

	// Destroy edges
	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(graph, node);

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

	++graph->version;
}

const hgraph_node_type_t*
hgraph_get_node_type(const hgraph_t* graph, hgraph_index_t id) {
	const hgraph_node_t* node = hgraph_find_node_by_id(graph, id);
	if (node == NULL) { return NULL; }

	return hgraph_get_node_type_internal(graph, node)->definition;
}

hgraph_index_t
hgraph_get_pin_id(
	const hgraph_t* graph,
	hgraph_index_t node_id,
	const hgraph_pin_description_t* pin
) {
	const hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return HGRAPH_INVALID_INDEX; }

	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(
		graph, node
	);

	for (hgraph_index_t i = 0; i < type_info->num_input_pins; ++i) {
		if (type_info->definition->input_pins[i] == pin) {
			return hgraph_encode_pin_id(node_id, i, false);
		}
	}

	for (hgraph_index_t i = 0; i < type_info->num_output_pins; ++i) {
		if (type_info->definition->output_pins[i] == pin) {
			return hgraph_encode_pin_id(node_id, i, true);
		}
	}

	return HGRAPH_INVALID_INDEX;
}

void
hgraph_resolve_pin(
	const hgraph_t* graph,
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

	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(
		graph, node
	);
	const hgraph_node_type_t* type_def = type_info->definition;

	*node_id_out = node_id;

	if (is_output && pin_index < type_info->num_output_pins) {
		*pin_desc_out = type_def->output_pins[pin_index];
	} else if (!is_output && pin_index < type_info->num_input_pins) {
		*pin_desc_out = type_def->input_pins[pin_index];
	} else {
		*pin_desc_out = NULL;
	}
}

bool
hgraph_can_connect(
	hgraph_t* graph,
	hgraph_index_t from_pin,
	hgraph_index_t to_pin
) {
	if (!(HGRAPH_IS_VALID_INDEX(from_pin) && HGRAPH_IS_VALID_INDEX(to_pin))) {
		return false;
	}

	bool is_output;
	hgraph_index_t from_node_id, from_pin_index;
	hgraph_decode_pin_id(from_pin, &from_node_id, &from_pin_index, &is_output);
	if (!is_output) { return false; }

	hgraph_index_t to_node_id, to_pin_index;
	hgraph_decode_pin_id(to_pin, &to_node_id, &to_pin_index, &is_output);
	if (is_output) { return false; }

	hgraph_node_t* from_node = hgraph_find_node_by_id(graph, from_node_id);
	hgraph_node_t* to_node = hgraph_find_node_by_id(graph, to_node_id);
	if ((from_node == NULL) || (to_node == NULL) || (from_node == to_node)) {
		return false;
	}

	const hgraph_node_type_info_t* from_type_info = hgraph_get_node_type_internal(
		graph, from_node
	);
	const hgraph_node_type_t* from_type_def = from_type_info->definition;
	if (from_pin_index >= from_type_info->num_output_pins) { return false; }

	const hgraph_node_type_info_t* to_type_info = hgraph_get_node_type_internal(
		graph, to_node
	);
	const hgraph_node_type_t* to_type_def = to_type_info->definition;
	if (to_pin_index >= to_type_info->num_input_pins) { return false; }

	const hgraph_data_type_t* from_data_type = from_type_def->output_pins[from_pin_index]->data_type;
	const hgraph_data_type_t* to_data_type = to_type_def->input_pins[to_pin_index]->data_type;
	if (from_data_type != to_data_type) { return false; }

	hgraph_index_t* input_pin = (hgraph_index_t*)((char*)to_node + to_type_info->input_pins[to_pin_index].offset);
	if (HGRAPH_IS_VALID_INDEX(*input_pin)) { return false; }

	return true;
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
	if ((from_node == NULL) || (to_node == NULL) || (from_node == to_node)) {
		return false;
	}

	const hgraph_node_type_info_t* from_type_info = hgraph_get_node_type_internal(
		graph, from_node
	);
	const hgraph_node_type_t* from_type_def = from_type_info->definition;
	if (from_pin_index >= from_type_info->num_output_pins) { return HGRAPH_INVALID_INDEX; }

	const hgraph_node_type_info_t* to_type_info = hgraph_get_node_type_internal(
		graph, to_node
	);
	const hgraph_node_type_t* to_type_def = to_type_info->definition;
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
	if (!HGRAPH_IS_VALID_INDEX(edge_slot)) { return; }

	hgraph_edge_t* edge = &graph->edges[edge_slot];
	bool is_output;
	hgraph_index_t from_node_id, from_pin_index, to_node_id, to_pin_index;
	hgraph_decode_pin_id(edge->from_pin, &from_node_id, &from_pin_index, &is_output);
	hgraph_decode_pin_id(edge->to_pin, &to_node_id, &to_pin_index, &is_output);

	hgraph_node_t* from_node = hgraph_find_node_by_id(graph, from_node_id);
	hgraph_node_t* to_node = hgraph_find_node_by_id(graph, to_node_id);
	HGRAPH_ASSERT((from_node != NULL) && (to_node != NULL));

	// Disconnect input pin
	const hgraph_node_type_info_t* to_type_info = hgraph_get_node_type_internal(
		graph, to_node
	);
	hgraph_index_t* input_pin = (hgraph_index_t*)((char*)to_node + to_type_info->input_pins[to_pin_index].offset);
	HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(*input_pin));
	*input_pin = HGRAPH_INVALID_INDEX;

	// Disconnect output pin
	const hgraph_node_type_info_t* from_type_info = hgraph_get_node_type_internal(
		graph, from_node
	);
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

HGRAPH_INTERNAL hgraph_str_t
hgraph_get_node_name(const hgraph_t* graph, hgraph_index_t node_id) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return (hgraph_str_t){ 0 }; }

	return hgraph_get_node_name_internal(graph, node);
}

void
hgraph_set_node_name(hgraph_t* graph, hgraph_index_t node_id, hgraph_str_t name) {
	if (name.length > graph->max_name_length) { return; }

	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return; }

	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(
		graph, node
	);

	char* name_storage = (char*)node + type_info->size;
	memcpy(name_storage, name.data, name.length);
	name_storage[name.length] = '\0';
	node->name_len = name.length;
}

hgraph_index_t
hgraph_get_node_by_name(const hgraph_t* graph, hgraph_str_t name) {
	for (hgraph_index_t i = 0; i < graph->node_slot_map.num_items; ++i) {
		hgraph_node_t* node = hgraph_get_node_by_slot(graph, i);
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

	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(
		graph, node
	);

	for (hgraph_index_t i = 0; i < type_info->num_attributes; ++i) {
		if (type_info->definition->attributes[i] == attribute) {
			char* storage = (char*)node + type_info->attributes[i].offset;
			memcpy(storage, value, attribute->data_type->size);
			break;
		}
	}
}

void*
hgraph_get_node_attribute(
	const hgraph_t* graph,
	hgraph_index_t node_id,
	const hgraph_attribute_description_t* attribute
) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return NULL; }

	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(
		graph, node
	);

	for (hgraph_index_t i = 0; i < type_info->num_attributes; ++i) {
		if (type_info->definition->attributes[i] == attribute) {
			return (char*)node + type_info->attributes[i].offset;
		}
	}

	return NULL;
}

void
hgraph_iterate_nodes(
	const hgraph_t* graph,
	hgraph_node_iterator_t iterator,
	void* userdata
) {
	for (hgraph_index_t i = 0; i < graph->node_slot_map.num_items; ++i) {
		const hgraph_node_t* node = hgraph_get_node_by_slot(graph, i);
		hgraph_index_t id = hgraph_slot_map_id_for_slot(&graph->node_slot_map, i);

		// May happen if removal occurs during iteration
		if (!HGRAPH_IS_VALID_INDEX(id)) { continue; }

		const hgraph_node_type_t* type = graph->registry->node_types[node->type].definition;
		if (!iterator(id, type, userdata)) { break; }
	}
}

void
hgraph_iterate_edges(
	const hgraph_t* graph,
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
	const hgraph_t* graph,
	hgraph_index_t node_id,
	hgraph_edge_iterator_t iterator,
	void* userdata
) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return; }

	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(
		graph, node
	);

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
	const hgraph_t* graph,
	hgraph_index_t node_id,
	hgraph_edge_iterator_t iterator,
	void* userdata
) {
	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) { return; }

	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(
		graph, node
	);

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

bool
hgraph_is_pin_connected(
	const hgraph_t* graph,
	hgraph_index_t pin_id
) {
	hgraph_index_t node_id, pin_index;
	bool is_output;
	hgraph_decode_pin_id(pin_id, &node_id, &pin_index, &is_output);

	hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
	if (node == NULL) {
		return false;
	}

	const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(
		graph, node
	);

	if (is_output) {
		if (pin_index >= type_info->num_output_pins) { return false; }

		const hgraph_var_t* var = &type_info->output_pins[pin_index];
		hgraph_edge_link_t* output_pin = (hgraph_edge_link_t*)((char*)node + var->offset);
		return HGRAPH_IS_VALID_INDEX(output_pin->next);
	} else {
		if (pin_index >= type_info->num_input_pins) { return false; }

		const hgraph_var_t* var = &type_info->input_pins[pin_index];
		hgraph_index_t* input_pin = (hgraph_index_t*)((char*)node + var->offset);
		return HGRAPH_IS_VALID_INDEX(*input_pin);
	}
}

hgraph_info_t
hgraph_get_info(const hgraph_t* graph) {
	return (hgraph_info_t){
		.num_nodes = graph->node_slot_map.num_items,
		.num_edges = graph->edge_slot_map.num_items,
	};
}
