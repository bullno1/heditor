#include "hgraph/runtime.h"
#include "internal.h"
#include "mem_layout.h"

size_t
hgraph_init(hgraph_t* graph, const hgraph_config_t* config) {
	mem_layout_t layout = { 0 };
	mem_layout_reserve(
		&layout,
		sizeof(hgraph_t),
		_Alignof(hgraph_t)
	);

	ptrdiff_t node_slot_map_offset = hgraph_slot_map_reserve(&layout, config->max_nodes);
	size_t node_size = config->registry->max_node_size + config->max_name_length;
	node_size = mem_layout_align_ptr((intptr_t)node_size, _Alignof(hgraph_node_t));

	ptrdiff_t nodes_offset = mem_layout_reserve(
		&layout,
		node_size * config->max_nodes,
		_Alignof(hgraph_node_t)
	);

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
hgraph_create_node(hgraph_t* graph, const hgraph_node_type_t* type);

void
hgraph_destroy_node(hgraph_t* graph, hgraph_index_t node);

hgraph_index_t
hgraph_get_pin_id(
	hgraph_t* graph,
	hgraph_index_t node,
	const hgraph_pin_description_t* pin
);

hgraph_index_t
hgraph_connect(
	hgraph_t* graph,
	hgraph_index_t from_pin,
	hgraph_index_t to_pin
);

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
	hgraph_index_t node,
	const hgraph_attribute_description_t* attribute,
	const void* value
);

const void*
hgraph_get_node_attribute(
	hgraph_t* graph,
	hgraph_index_t node,
	const hgraph_attribute_description_t* attribute
);

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
