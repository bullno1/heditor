#ifndef HGRAPH_INTERNAL_H
#define HGRAPH_INTERNAL_H

#include <hgraph/runtime.h>
#include "ptr_table.h"
#include "slot_map.h"
#include "assert.h"
#include <string.h>

#define HGRAPH_PRIVATE static inline
#define HGRAPH_INTERNAL
#define HGRAPH_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define HGRAPH_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define HGRAPH_ASSERT assert
#define HGRAPH_CONTAINER_OF(PTR, TYPE, MEMBER) \
    (TYPE*)((char *)(PTR) - offsetof(TYPE, MEMBER))

struct hgraph_registry_builder_s {
	hgraph_registry_config_t config;
	hgraph_plugin_api_t plugin_api;
	hgraph_index_t num_node_types;
	hgraph_index_t num_data_types;
	hgraph_index_t data_type_exp;
	const hgraph_node_type_t** node_types;
	const hgraph_data_type_t** data_types;
};

typedef struct hgraph_data_type_info_s {
	hgraph_str_t name;
	size_t size;
	const hgraph_data_type_t* definition;
} hgraph_data_type_info_t;

typedef struct hgraph_var_s {
	hgraph_index_t type;
	hgraph_str_t name;
	ptrdiff_t offset;
} hgraph_var_t;

typedef struct hgraph_output_buffer_info_s {
	ptrdiff_t offset;
} hgraph_output_buffer_info_t;

typedef struct hgraph_node_type_info_s {
	hgraph_str_t name;
	const hgraph_node_type_t* definition;

	size_t size;
	size_t pipeline_data_size;

	hgraph_index_t num_attributes;
	hgraph_var_t* attributes;

	hgraph_index_t num_input_pins;
	hgraph_var_t* input_pins;

	hgraph_index_t num_output_pins;
	hgraph_var_t* output_pins;
	hgraph_output_buffer_info_t* output_buffers;
} hgraph_node_type_info_t;

struct hgraph_registry_s {
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
	hgraph_index_t prev;
	hgraph_index_t next;
} hgraph_edge_link_t;

struct hgraph_edge_s {
	hgraph_edge_link_t output_pin_link;

	hgraph_index_t from_pin;
	hgraph_index_t to_pin;
};

typedef struct hgraph_node_s {
	size_t name_len;
	hgraph_index_t type;
} hgraph_node_t;

struct hgraph_s {
	const hgraph_registry_t* registry;
	hgraph_config_t config;
	hgraph_index_t version;

	hgraph_slot_map_t node_slot_map;
	size_t node_size;
	hgraph_index_t* node_versions;
	char* nodes;

	hgraph_slot_map_t edge_slot_map;
	hgraph_edge_t* edges;
};

typedef enum hgraph_node_pipeline_state_s {
	HGRAPH_NODE_STATE_WAITING,
	HGRAPH_NODE_STATE_SCHEDULED,
	HGRAPH_NODE_STATE_EXECUTED,
} hgraph_node_pipeline_state_t;

typedef struct hgraph_pipeline_node_meta_s {
	hgraph_index_t id;
	hgraph_index_t version;
	hgraph_index_t type;
	char* data;
	void* status;
	hgraph_node_pipeline_state_t state;
} hgraph_pipeline_node_meta_t;

struct hgraph_pipeline_s {
	hgraph_index_t version;
	hgraph_pipeline_stats_t stats;
	const hgraph_t* graph;

	hgraph_index_t num_nodes;
	hgraph_index_t num_ready_nodes;
	hgraph_index_t* ready_nodes;

	hgraph_pipeline_node_meta_t* node_metas;

	char* scratch_zone_start;
	char* scratch_zone_end;
	char* step_alloc_ptr;
	char* execution_alloc_ptr;
};

typedef struct hgraph_var_migration_plan_s {
	hgraph_index_t new_index;
} hgraph_var_migration_plan_t;

typedef struct hgraph_node_migration_plan_s {
	const hgraph_node_type_info_t* new_type;
	hgraph_var_migration_plan_t* attribute_plans;
	hgraph_var_migration_plan_t* input_plans;
	hgraph_var_migration_plan_t* output_plans;
} hgraph_node_migration_plan_t;

struct hgraph_migration_s {
	hgraph_node_migration_plan_t* node_plans;
};

extern const hgraph_node_type_t hgraph_dummy_node;

HGRAPH_PRIVATE bool
hgraph_str_equal(hgraph_str_t lhs, hgraph_str_t rhs) {
	if (lhs.length != rhs.length) { return false; }

	return memcmp(lhs.data, rhs.data, lhs.length) == 0;
}

#endif
