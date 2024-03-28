#ifndef HGRAPH_INTERNAL_H
#define HGRAPH_INTERNAL_H

#define HGRAPH_PRIVATE static inline
#define HGRAPH_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define HGRAPH_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#include <hgraph/runtime.h>
#include "ptr_table.h"
#include "slot_map.h"

struct hgraph_registry_builder_s {
	hgraph_registry_config_t config;
	hgraph_index_t num_node_types;
	hgraph_index_t num_data_types;
	hgraph_index_t data_type_exp;
	const hgraph_node_type_t** node_types;
	const hgraph_data_type_t** data_types;
};

typedef struct hgraph_data_type_info_s {
	hgraph_str_t name;
	const hgraph_data_type_t* definition;
} hgraph_data_type_info_t;

typedef struct hgraph_var_s {
	hgraph_str_t type;
	hgraph_str_t name;
	ptrdiff_t offset;
} hgraph_var_t;

typedef struct hgraph_node_type_info_s {
	hgraph_str_t name;
	const hgraph_node_type_t* definition;

	size_t size;

	hgraph_index_t num_attributes;
	hgraph_var_t* attributes;

	hgraph_index_t num_input_pins;
	hgraph_var_t* input_pins;

	hgraph_index_t num_output_pins;
	hgraph_var_t* output_pins;
} hgraph_node_type_info_t;

struct hgraph_registry_s {
	hgraph_registry_config_t config;

	size_t max_node_size;
	hgraph_index_t max_edges_per_node;

	hgraph_index_t num_data_types;
	hgraph_data_type_info_t* data_types;

	hgraph_index_t num_node_types;
	hgraph_node_type_info_t* node_types;

	hgraph_ptr_table_t data_type_by_definition;
	hgraph_ptr_table_t node_type_by_definition;
};

typedef struct hgraph_edge_s hgraph_edge_t;

typedef struct hgraph_edge_link_s {
	hgraph_edge_t* prev;
	hgraph_edge_t* next;
} hgraph_edge_link_t;

struct hgraph_edge_s {
	hgraph_edge_link_t node_out_edges_link;

	hgraph_index_t from_pin;
	hgraph_index_t to_pin;
};

typedef struct hgraph_node_s {
	size_t name_len;
	hgraph_index_t type;
} hgraph_node_t;

struct hgraph_s {
	hgraph_config_t config;

	hgraph_slot_map_t node_slot_map;
	size_t node_size;
	char* nodes;

	hgraph_slot_map_t edge_slot_map;
	hgraph_edge_t* edges;
};

#endif
