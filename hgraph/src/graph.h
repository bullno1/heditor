#ifndef HGRAPH_GRAPH_INTERNAL_H
#define HGRAPH_GRAPH_INTERNAL_H

#include "internal.h"

HGRAPH_INTERNAL hgraph_str_t
hgraph_get_node_name_internal(const hgraph_t* graph, const hgraph_node_t* node);

HGRAPH_INTERNAL void
hgraph_find_node_type(
	const hgraph_t* graph,
	const hgraph_node_t* node,
	const hgraph_node_type_info_t** info,
	const hgraph_node_type_t** def
);

HGRAPH_INTERNAL hgraph_node_t*
hgraph_find_node_by_id(
	const hgraph_t* graph,
	hgraph_index_t node_id
);

HGRAPH_INTERNAL hgraph_index_t
hgraph_encode_pin_id(
	hgraph_index_t node_id,
	hgraph_index_t pin_index,
	bool is_output
);

HGRAPH_INTERNAL void
hgraph_decode_pin_id(
	hgraph_index_t pin_id,
	hgraph_index_t* node_id,
	hgraph_index_t* pin_index,
	bool* is_output
);

#endif
