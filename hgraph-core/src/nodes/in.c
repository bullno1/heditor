#include "hgraph/plugin.h"
#include <hgraph/core.h>

// f32
static const hgraph_pin_description_t hgraph_core_f32_in_in = {
	.name = HGRAPH_STR("in"),
	.label = HGRAPH_STR("Input"),
	.data_type = &hgraph_core_f32,
};

static inline void
hgraph_core_f32_in_execute(const hgraph_node_api_t* api) {
	const float* value = hgraph_node_input(
		api,
		&hgraph_core_f32_in_in
	);
	hgraph_node_report_status(api, value);
}

const hgraph_node_type_t hgraph_core_f32_in = {
	.name = HGRAPH_STR("core.in.f32"),
	.label = HGRAPH_STR("Float in"),
	.description = HGRAPH_STR("Accept a single float"),
	.group = HGRAPH_STR("Core/Input"),
	.size = 0,
	.alignment = _Alignof(char),
	.input_pins = HGRAPH_NODE_PINS(
		&hgraph_core_f32_in_in
	),
	.execute = hgraph_core_f32_in_execute,
};

// i32
static const hgraph_pin_description_t hgraph_core_i32_in_in = {
	.name = HGRAPH_STR("in"),
	.label = HGRAPH_STR("Input"),
	.data_type = &hgraph_core_i32,
};

static inline void
hgraph_core_i32_in_execute(const hgraph_node_api_t* api) {
	const int32_t* value = hgraph_node_input(
		api,
		&hgraph_core_i32_in_in
	);
	hgraph_node_report_status(api, value);
}

const hgraph_node_type_t hgraph_core_i32_in = {
	.name = HGRAPH_STR("core.in.i32"),
	.label = HGRAPH_STR("Integer in"),
	.description = HGRAPH_STR("Accept a single integer"),
	.group = HGRAPH_STR("Core/Input"),
	.size = 0,
	.alignment = _Alignof(char),
	.input_pins = HGRAPH_NODE_PINS(
		&hgraph_core_i32_in_in
	),
	.execute = hgraph_core_i32_in_execute,
};

// str
static const hgraph_pin_description_t hgraph_core_str_in_in = {
	.name = HGRAPH_STR("in"),
	.label = HGRAPH_STR("Input"),
	.data_type = &hgraph_core_var_str,
};

static inline void
hgraph_core_str_in_execute(const hgraph_node_api_t* api) {
	const hgraph_str_t* value = (const hgraph_str_t*)hgraph_node_input(
		api,
		&hgraph_core_str_in_in
	);
	hgraph_node_report_status(api, value);
}

const hgraph_node_type_t hgraph_core_str_in = {
	.name = HGRAPH_STR("core.in.str"),
	.label = HGRAPH_STR("String in"),
	.description = HGRAPH_STR("Accept a single string"),
	.group = HGRAPH_STR("Core/Input"),
	.size = 0,
	.alignment = _Alignof(char),
	.input_pins = HGRAPH_NODE_PINS(
		&hgraph_core_str_in_in
	),
	.execute = hgraph_core_str_in_execute,
};
