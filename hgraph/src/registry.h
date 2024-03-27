#ifndef HGRAPH_REGISTRY_H
#define HGRAPH_REGISTRY_H

#include <hgraph/runtime.h>
#include "ptr_table.h"

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
} hgraph_var_t;

typedef struct hgraph_node_type_info_s {
	hgraph_str_t name;
	const hgraph_node_type_t* definition;

	hgraph_index_t num_attributes;
	hgraph_index_t num_input_pins;
	hgraph_index_t num_output_pins;

	hgraph_var_t* attributes;
	hgraph_var_t* input_pins;
	hgraph_var_t* output_pins;
} hgraph_node_type_info_t;

struct hgraph_registry_s {
	hgraph_registry_config_t config;
	hgraph_index_t num_data_types;
	hgraph_index_t num_node_types;

	hgraph_data_type_info_t* data_types;
	hgraph_node_type_info_t* node_types;

	hgraph_ptr_table_t data_type_by_definition;
	hgraph_ptr_table_t node_type_by_definition;
};

#endif
