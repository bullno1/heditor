#ifndef HGRAPH_RUNTIME_H
#define HGRAPH_RUNTIME_H

#include "common.h"
#include "plugin.h"
#include <stdbool.h>

#ifdef HGRAPH_RUNTIME_SHARED
#	if defined(_WIN32) && !defined(__MINGW32__)
#		ifdef HGRAPH_RUNTIME_BUILD
#			define HGRAPH_API __declspec(dllexport)
#		else
#			define HGRAPH_API __declspec(dllimport)
#		endif
#	else
#		ifdef HGRAPH_RUNTIME_BUILD
#			define HGRAPH_API __attribute__((visibility("default")))
#		else
#			define HGRAPH_API extern
#		endif
#	endif
#else
#	define HGRAPH_API extern
#endif

typedef struct hgraph_registry_builder_s hgraph_registry_builder_t;
typedef struct hgraph_registry_s hgraph_registry_t;
typedef struct hgraph_s hgraph_t;
typedef struct hgraph_migration_s hgraph_migration_t;
typedef struct hgraph_pipeline_s hgraph_pipeline_t;

typedef struct hgraph_registry_config_s {
	hgraph_index_t max_data_types;
	hgraph_index_t max_node_types;
} hgraph_registry_config_t;

typedef struct hgraph_config_s {
	hgraph_index_t max_nodes;
	hgraph_index_t max_name_length;
} hgraph_config_t;

typedef struct hgraph_pipeline_config_s {
	size_t max_nodes;
	size_t max_node_size;
	size_t max_scratch_memory;
} hgraph_pipeline_config_t;

typedef enum hgraph_pipeline_event_type_e {
	HGRAPH_PIPELINE_EV_BEGIN_PIPELINE,
	HGRAPH_PIPELINE_EV_BEGIN_NODE,
	HGRAPH_PIPELINE_EV_END_NODE,
	HGRAPH_PIPELINE_EV_END_PIPELINE,
} hgraph_pipeline_event_type_t;

typedef enum hgraph_pipeline_execution_status_e {
	HGRAPH_PIPELINE_EXEC_FINISHED,
	HGRAPH_PIPELINE_EXEC_INTERRUPTED,
	HGRAPH_PIPELINE_EXEC_OOM,
	HGRAPH_PIPELINE_EXEC_NEED_REINIT,
} hgraph_pipeline_execution_status_t;

typedef struct hgraph_header_s {
	hgraph_index_t version;
} hgraph_header_t;

typedef struct hgraph_pipeline_event_s {
	hgraph_pipeline_event_type_t type;
	hgraph_index_t node;
} hgraph_pipeline_event_t;

typedef struct hgraph_pipeline_stats_s {
	size_t peak_step_memory;
	size_t peak_execution_memory;
	size_t peak_persistent_memory;
} hgraph_pipeline_stats_t;

typedef bool (*hgraph_registry_iterator_t)(
	const hgraph_node_type_t* node_type,
	void* userdata
);

typedef bool (*hgraph_node_iterator_t)(
	hgraph_index_t node,
	const hgraph_node_type_t* node_type,
	void* userdata
);

typedef bool (*hgraph_edge_iterator_t)(
	hgraph_index_t edge,
	hgraph_index_t from_pin,
	hgraph_index_t to_pin,
	void* userdata
);

typedef bool (*hgraph_pipeline_watcher_t)(
	const hgraph_pipeline_event_t* event, void* userdata
);

#ifdef __cplusplus
extern "C" {
#endif

HGRAPH_API hgraph_registry_builder_t*
hgraph_registry_builder_init(
	const hgraph_registry_config_t* config,
	void* mem,
	size_t* mem_size_inout
);

HGRAPH_API void
hgraph_registry_builder_add(
	hgraph_registry_builder_t* builder,
	const hgraph_node_type_t* type
);

HGRAPH_API hgraph_plugin_api_t*
hgraph_registry_builder_as_plugin_api(hgraph_registry_builder_t* builder);

HGRAPH_API hgraph_registry_t*
hgraph_registry_init(
	const hgraph_registry_builder_t* builder,
	void* mem,
	size_t* mem_size_inout
);

HGRAPH_API void
hgraph_registry_iterate(
	const hgraph_registry_t* registry,
	hgraph_registry_iterator_t iterator,
	void* userdata
);

HGRAPH_API hgraph_t*
hgraph_init(
	const hgraph_registry_t* registry,
	const hgraph_config_t* config,
	void* mem,
	size_t* mem_size_inout
);

HGRAPH_API hgraph_migration_t*
hgraph_migration_init(
	const hgraph_registry_t* from_registry,
	const hgraph_registry_t* to_registry,
	void* mem,
	size_t* mem_size_inout
);

HGRAPH_API void
hgraph_migration_execute(
	const hgraph_migration_t* migration,
	const hgraph_t* from_graph,
	hgraph_t* to_graph
);

HGRAPH_API hgraph_index_t
hgraph_create_node(hgraph_t* graph, const hgraph_node_type_t* type);

HGRAPH_API void
hgraph_destroy_node(hgraph_t* graph, hgraph_index_t node);

HGRAPH_API const hgraph_node_type_t*
hgraph_get_node_type(const hgraph_t* graph, hgraph_index_t node);

HGRAPH_API hgraph_index_t
hgraph_get_pin_id(
	const hgraph_t* graph,
	hgraph_index_t node,
	const hgraph_pin_description_t* pin
);

HGRAPH_API void
hgraph_resolve_pin(
	const hgraph_t* graph,
	hgraph_index_t pin_id,
	hgraph_index_t* node_id_out,
	const hgraph_pin_description_t** pin_desc_out
);

HGRAPH_API hgraph_index_t
hgraph_connect(
	hgraph_t* graph,
	hgraph_index_t from_pin,
	hgraph_index_t to_pin
);

HGRAPH_API void
hgraph_disconnect(hgraph_t* graph, hgraph_index_t edge);

HGRAPH_API hgraph_str_t
hgraph_get_node_name(const hgraph_t* graph, hgraph_index_t node);

HGRAPH_API void
hgraph_set_node_name(hgraph_t* graph, hgraph_index_t node, hgraph_str_t name);

HGRAPH_API hgraph_index_t
hgraph_get_node_by_name(const hgraph_t* graph, hgraph_str_t name);

HGRAPH_API void
hgraph_set_node_attribute(
	hgraph_t* graph,
	hgraph_index_t node,
	const hgraph_attribute_description_t* attribute,
	const void* value
);

HGRAPH_API const void*
hgraph_get_node_attribute(
	const hgraph_t* graph,
	hgraph_index_t node,
	const hgraph_attribute_description_t* attribute
);

HGRAPH_API void
hgraph_iterate_nodes(
	const hgraph_t* graph,
	hgraph_node_iterator_t iterator,
	void* userdata
);

HGRAPH_API void
hgraph_iterate_edges(
	const hgraph_t* graph,
	hgraph_edge_iterator_t iterator,
	void* userdata
);

HGRAPH_API void
hgraph_iterate_edges_to(
	const hgraph_t* graph,
	hgraph_index_t node_id,
	hgraph_edge_iterator_t iterator,
	void* userdata
);

HGRAPH_API void
hgraph_iterate_edges_from(
	const hgraph_t* graph,
	hgraph_index_t node_id,
	hgraph_edge_iterator_t iterator,
	void* userdata
);

HGRAPH_API size_t
hgraph_pipeline_init(
	hgraph_pipeline_t* pipeline,
	const hgraph_pipeline_config_t* config
);

HGRAPH_API void
hgraph_pipeline_cleanup(hgraph_pipeline_t* pipeline);

HGRAPH_API hgraph_pipeline_execution_status_t
hgraph_pipeline_execute(
	hgraph_pipeline_t* pipeline,
	hgraph_pipeline_watcher_t watcher,
	void* userdata
);

HGRAPH_API const void*
hgraph_pipeline_get_node_status(
	hgraph_pipeline_t* pipeline,
	hgraph_index_t node
);

HGRAPH_API hgraph_pipeline_stats_t
hgraph_pipeline_get_stats(hgraph_pipeline_t* pipeline);

HGRAPH_API void
hgraph_pipeline_reset_stats(hgraph_pipeline_t* pipeline);

HGRAPH_API void
hgraph_pipeline_render(hgraph_pipeline_t* pipeline, void* render_ctx);

HGRAPH_API hgraph_io_status_t
hgraph_write_header(hgraph_out_t* out);

HGRAPH_API hgraph_io_status_t
hgraph_read_header(hgraph_header_t* header, hgraph_in_t* in);

HGRAPH_API hgraph_io_status_t
hgraph_write_graph(const hgraph_t* graph, hgraph_out_t* out);

HGRAPH_API hgraph_io_status_t
hgraph_read_graph_config(
	const hgraph_header_t* header,
	hgraph_config_t* config,
	hgraph_in_t* input
);

HGRAPH_API hgraph_io_status_t
hgraph_read_graph(
	const hgraph_header_t* header,
	hgraph_t* graph,
	hgraph_in_t* input
);

#ifdef __cplusplus
}
#endif

#endif
