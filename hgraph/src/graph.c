#include "internal.h"
#include "mem_layout.h"
#include "ptr_table.h"
#include <string.h>

size_t
hgraph_init(hgraph_t* graph, const hgraph_config_t* config) {
	mem_layout_t layout = { 0 };
	mem_layout_reserve(
		&layout,
		sizeof(hgraph_t),
		_Alignof(hgraph_t)
	);

	size_t node_size = config->registry->max_node_size + config->max_name_length;
	node_size = mem_layout_align_ptr((intptr_t)node_size, _Alignof(hgraph_node_t));
	ptrdiff_t nodes_offset = mem_layout_reserve(
		&layout,
		node_size * config->max_nodes,
		_Alignof(max_align_t)
	);

	ptrdiff_t node_slot_map_offset = hgraph_slot_map_reserve(&layout, config->max_nodes);

	size_t max_edges = config->max_nodes * config->registry->max_edges_per_node;
	ptrdiff_t edge_slot_map_offset = hgraph_slot_map_reserve(&layout, max_edges);
	ptrdiff_t edges_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_edge_t) * max_edges,
		_Alignof(hgraph_edge_t)
	);

	size_t size = mem_layout_size(&layout);
	if (graph == NULL) { return size; }

	*graph = (hgraph_t){
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

	return size;
}

size_t
hgraph_migrate(
	hgraph_t* dest,
	const hgraph_t* src,
	hgraph_registry_t* new_registry
);

hgraph_index_t
hgraph_create_node(hgraph_t* graph, const hgraph_node_type_t* type) {
	const hgraph_registry_t* registry = graph->config.registry;
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
	if (!HGRAPH_IS_VALID_INDEX(id)) { return; }

	hgraph_index_t src_slot, dst_slot;
	hgraph_slot_map_free(&graph->node_slot_map, id, &dst_slot, &src_slot);
	if (!HGRAPH_IS_VALID_INDEX(src_slot)) { return; }

	size_t node_size = graph->node_size;
	char* src_node = graph->nodes + node_size * src_slot;
	char* dst_node = graph->nodes + node_size  * dst_slot;
	memcpy(dst_node, src_node, node_size);
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

HGRAPH_PRIVATE hgraph_node_t*
hgraph_find_node_by_id(
	const hgraph_t* graph,
	hgraph_index_t node_id
) {
	hgraph_index_t node_slot = hgraph_slot_map_slot_for_id(&graph->node_slot_map, node_id);
	if (!HGRAPH_IS_VALID_INDEX(node_slot)) { return NULL; }

	return (hgraph_node_t*)(graph->nodes + graph->node_size * node_slot);
}

HGRAPH_PRIVATE void
hgraph_find_node_type(
	const hgraph_t* graph,
	const hgraph_node_t* node,
	const hgraph_node_type_info_t** info,
	const hgraph_node_type_t** def
) {
	const hgraph_registry_t* registry = graph->config.registry;
	const hgraph_node_type_info_t* type_info = &registry->node_types[node->type];
	const hgraph_node_type_t* type_def = type_info->definition;

	*info = type_info;
	*def = type_def;
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

	if (from_type_def != to_type_def) { return HGRAPH_INVALID_INDEX; }

	hgraph_index_t* in_edge = (hgraph_index_t*)((char*)to_node + to_type_info->input_pins[to_pin_index].offset);
	if (HGRAPH_IS_VALID_INDEX(*in_edge)) { return HGRAPH_INVALID_INDEX; }

	hgraph_edge_link_t* out_edge = (hgraph_edge_link_t*)((char*)from_node + from_type_info->output_pins[from_pin_index].offset);

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

	*in_edge = edge_id;

	edge->out_edge_link.prev = out_edge->prev;
	edge->out_edge_link.next = HGRAPH_INVALID_INDEX;
	hgraph_edge_link_t* prev = hgraph_edge_prev(graph, out_edge);
	prev->next = edge_id;
	out_edge->prev = edge_id;
}

void
hgraph_disconnect(hgraph_t* graph, hgraph_index_t edge);

hgraph_str_t
hgraph_get_node_name(hgraph_t* graph, hgraph_index_t node);

void
hgraph_set_node_name(hgraph_t* graph, hgraph_index_t node, hgraph_str_t name);

hgraph_index_t
hgraph_get_node_by_name(hgraph_t* graph, hgraph_str_t name);

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
hgraph_iterate_node(
	hgraph_t* graph,
	hgraph_node_iterator_t iterator,
	void* userdata
);

void
hgraph_iterate_edge(
	hgraph_t* graph,
	hgraph_edge_iterator_t iterator,
	void* userdata
);
