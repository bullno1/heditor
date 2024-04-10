#ifndef HGRAPH_CORE_H
#define HGRAPH_CORE_H

#include <hgraph/plugin.h>
#include <hgraph/runtime.h>
#include <string.h>

#ifdef HGRAPH_CORE_SHARED
#	if defined(_WIN32) && !defined(__MINGW32__)
#		ifdef HGRAPH_CORE_BUILD
#			define HGRAPH_CORE_API __declspec(dllexport)
#		else
#			define HGRAPH_CORE_API __declspec(dllimport)
#		endif
#	else
#		ifdef HGRAPH_CORE_BUILD
#			define HGRAPH_CORE_API __attribute__((visibility("default")))
#		else
#			define HGRAPH_CORE_API extern
#		endif
#	endif
#else
#	define HGRAPH_CORE_API extern
#endif

#define HGRAPH_CORE_FIXED_STR_LEN 512

typedef struct hgraph_core_fixed_str_s {
	size_t len;
	char data[HGRAPH_CORE_FIXED_STR_LEN];
} hgraph_core_fixed_str_t;

typedef enum hgraph_core_rounding_mode_e {
	HGRAPH_CORE_ROUND_NEAREST = 0,
	HGRAPH_CORE_ROUND_FLOOR = 1,
	HGRAPH_CORE_ROUND_CEIL = 2,
} hgraph_core_rounding_mode_t;

#ifdef __cplusplus
extern "C" {
#endif

// Types

HGRAPH_CORE_API extern const hgraph_data_type_t hgraph_core_f32;
HGRAPH_CORE_API extern const hgraph_data_type_t hgraph_core_i32;
HGRAPH_CORE_API extern const hgraph_data_type_t hgraph_core_fixed_str;
HGRAPH_CORE_API extern const hgraph_data_type_t hgraph_core_var_str;

// Nodes

// Simple output nodes

HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_f32_out;
HGRAPH_CORE_API extern const hgraph_attribute_description_t hgraph_core_f32_out_attr_value;
HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_i32_out;
HGRAPH_CORE_API extern const hgraph_attribute_description_t hgraph_core_i32_out_attr_value;
HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_fixed_str_out;
HGRAPH_CORE_API extern const hgraph_attribute_description_t hgraph_core_fixed_str_out_attr_value;
HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_var_str_out;
HGRAPH_CORE_API extern const hgraph_attribute_description_t hgraph_core_var_str_out_attr_value;

// Simple input nodes

HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_f32_in;
HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_i32_in;
HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_str_in;

// Simple conversion nodes

HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_i32_to_f32;
HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_f32_to_i32;
HGRAPH_CORE_API extern const hgraph_attribute_description_t hgraph_core_f32_to_i32_attr_rounding_mode;
HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_i32_to_str;
HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_f32_to_str;
HGRAPH_CORE_API extern const hgraph_node_type_t hgraph_core_str_concat;

// Manual registration

HGRAPH_CORE_API void
hgraph_core_register(const hgraph_plugin_api_t* plugin_api);

// Type safe manipulation functions

static inline void
hgraph_core_set_i32_out_value(
	hgraph_t* graph,
	hgraph_index_t node,
	int32_t value
) {
	hgraph_set_node_attribute(
		graph,
		node,
		&hgraph_core_i32_out_attr_value,
		&value
	);
}

static inline void
hgraph_core_set_f32_out_value(
	hgraph_t* graph,
	hgraph_index_t node,
	float value
) {
	hgraph_set_node_attribute(
		graph,
		node,
		&hgraph_core_f32_out_attr_value,
		&value
	);
}

static inline void
hgraph_core_set_str_out_value(
	hgraph_t* graph,
	hgraph_index_t node,
	hgraph_str_t value
) {
	const hgraph_node_type_t* type = hgraph_get_node_type(graph, node);
	if (type == &hgraph_core_fixed_str_out) {
		hgraph_core_fixed_str_t tmp;
		tmp.len = value.length <= HGRAPH_CORE_FIXED_STR_LEN
			? value.length
			: HGRAPH_CORE_FIXED_STR_LEN;
		memcpy(tmp.data, value.data, tmp.len);

		hgraph_set_node_attribute(
			graph,
			node,
			&hgraph_core_fixed_str_out_attr_value,
			&tmp
		);
	} else if (type == &hgraph_core_var_str_out) {
		hgraph_set_node_attribute(
			graph,
			node,
			&hgraph_core_var_str_out_attr_value,
			&value
		);
	}
}

static inline void
hgraph_core_set_f32_to_i32_rounding_mode(
	hgraph_t* graph,
	hgraph_index_t node,
	hgraph_core_rounding_mode_t mode
) {
	hgraph_set_node_attribute(
		graph,
		node,
		&hgraph_core_f32_to_i32_attr_rounding_mode,
		&(int32_t){ (int32_t)mode }
	);
}

static inline float
hgraph_core_get_f32_in_result(
	hgraph_pipeline_t* pipeline,
	hgraph_index_t node
) {
	const float* ptr = (const float*)hgraph_pipeline_get_node_status(pipeline, node);
	return ptr != NULL ? *ptr : 0.0f;
}

static inline int32_t
hgraph_core_get_i32_in_result(
	hgraph_pipeline_t* pipeline,
	hgraph_index_t node
) {
	const int32_t* ptr = (const int32_t*)hgraph_pipeline_get_node_status(pipeline, node);
	return ptr != NULL ? *ptr : 0;
}

static inline hgraph_str_t
hgraph_core_get_str_in_result(
	hgraph_pipeline_t* pipeline,
	hgraph_index_t node
) {
	const hgraph_str_t* ptr = (const hgraph_str_t*)hgraph_pipeline_get_node_status(pipeline, node);
	return ptr != NULL ? *ptr : (hgraph_str_t){ 0 };
}

#ifdef __cplusplus
}
#endif

#endif
