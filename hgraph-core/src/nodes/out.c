#include "hgraph/plugin.h"
#include <hgraph/core.h>

// f32
static const hgraph_pin_description_t hgraph_core_f32_out_out = {
	.name = HGRAPH_STR("core.out.f32.out"),
	.label = HGRAPH_STR("Output"),
	.data_type = &hgraph_core_f32,
};

const hgraph_attribute_description_t hgraph_core_f32_out_attr_value = {
	.name = HGRAPH_STR("core.out.f32.value"),
	.label = HGRAPH_STR("Value"),
	.description = HGRAPH_STR("The value to output"),
	.data_type = &hgraph_core_f32,
};

static inline void
hgraph_core_f32_out_execute(const hgraph_node_api_t* api) {
	float value = *(const float*)hgraph_node_input(
		api,
		&hgraph_core_f32_out_attr_value
	);
	hgraph_node_output(api, &hgraph_core_f32_out_out, &value);
}

const hgraph_node_type_t hgraph_core_f32_out = {
	.name = HGRAPH_STR("core.out.f32"),
	.label = HGRAPH_STR("Float out"),
	.description = HGRAPH_STR("Output a single float"),
	.group = HGRAPH_STR("Core.Output"),
	.size = 0,
	.alignment = _Alignof(char),
	.attributes = HGRAPH_NODE_ATTRIBUTES(
		&hgraph_core_f32_out_attr_value
	),
	.output_pins = HGRAPH_NODE_PINS(
		&hgraph_core_f32_out_out
	),
	.execute = hgraph_core_f32_out_execute,
};

// i32
static const hgraph_pin_description_t hgraph_core_i32_out_out = {
	.name = HGRAPH_STR("core.out.i32.out"),
	.label = HGRAPH_STR("Output"),
	.data_type = &hgraph_core_i32,
};

const hgraph_attribute_description_t hgraph_core_i32_out_attr_value = {
	.name = HGRAPH_STR("core.out.i32.value"),
	.label = HGRAPH_STR("Value"),
	.description = HGRAPH_STR("The value to output"),
	.data_type = &hgraph_core_i32,
};

static inline void
hgraph_core_i32_out_execute(const hgraph_node_api_t* api) {
	int32_t value = *(const int32_t*)hgraph_node_input(api, &hgraph_core_i32_out_attr_value);
	hgraph_node_output(api, &hgraph_core_i32_out_out, &value);
}

const hgraph_node_type_t hgraph_core_i32_out = {
	.name = HGRAPH_STR("core.out.i32"),
	.label = HGRAPH_STR("Integer out"),
	.description = HGRAPH_STR("Output a single integer"),
	.group = HGRAPH_STR("Core.Output"),
	.size = 0,
	.alignment = _Alignof(char),
	.attributes = HGRAPH_NODE_ATTRIBUTES(
		&hgraph_core_i32_out_attr_value
	),
	.output_pins = HGRAPH_NODE_PINS(
		&hgraph_core_i32_out_out
	),
	.execute = hgraph_core_i32_out_execute,
};

// fixed string
static const hgraph_pin_description_t hgraph_core_fixed_str_out_out = {
	.name = HGRAPH_STR("core.out.fixed_str.out"),
	.label = HGRAPH_STR("Output"),
	.data_type = &hgraph_core_var_str,
};

const hgraph_attribute_description_t hgraph_core_fixed_str_out_attr_value = {
	.name = HGRAPH_STR("core.out.fixed_str.value"),
	.label = HGRAPH_STR("Value"),
	.description = HGRAPH_STR("The value to output"),
	.data_type = &hgraph_core_fixed_str,
};

static inline void
hgraph_core_fixed_str_out_execute(const hgraph_node_api_t* api) {
	const hgraph_core_fixed_str_t* value = (const hgraph_core_fixed_str_t*)hgraph_node_input(
		api,
		&hgraph_core_fixed_str_out_attr_value
	);
	char* chars = hgraph_node_allocate(api, HGRAPH_LIFETIME_EXECUTION, value->len);
	memcpy(chars, value->data, value->len);
	hgraph_str_t out = { .length = value->len, .data = chars };
	hgraph_node_output(api, &hgraph_core_fixed_str_out_out, &out);
}

const hgraph_node_type_t hgraph_core_fixed_str_out = {
	.name = HGRAPH_STR("core.out.fixed_str"),
	.label = HGRAPH_STR("String out"),
	.description = HGRAPH_STR("Output a single string"),
	.group = HGRAPH_STR("Core.Output"),
	.size = 0,
	.alignment = _Alignof(char),
	.attributes = HGRAPH_NODE_ATTRIBUTES(
		&hgraph_core_fixed_str_out_attr_value
	),
	.output_pins = HGRAPH_NODE_PINS(
		&hgraph_core_fixed_str_out_out
	),
	.execute = hgraph_core_fixed_str_out_execute,
};

// variable string
static const hgraph_pin_description_t hgraph_core_var_str_out_out = {
	.name = HGRAPH_STR("core.out.var_str.out"),
	.label = HGRAPH_STR("Output"),
	.data_type = &hgraph_core_var_str,
};

const hgraph_attribute_description_t hgraph_core_var_str_out_attr_value = {
	.name = HGRAPH_STR("core.out.var_str.value"),
	.label = HGRAPH_STR("Value"),
	.description = HGRAPH_STR("The value to output"),
	.data_type = &hgraph_core_var_str,
};

static inline void
hgraph_core_var_str_out_execute(const hgraph_node_api_t* api) {
	const hgraph_str_t value = *(const hgraph_str_t*)hgraph_node_input(
		api,
		&hgraph_core_var_str_out_attr_value
	);
	char* chars = hgraph_node_allocate(api, HGRAPH_LIFETIME_EXECUTION, value.length);
	memcpy(chars, value.data, value.length);
	hgraph_str_t out = { .length = value.length, .data = chars };
	hgraph_node_output(api, &hgraph_core_var_str_out_out, &out);
}

const hgraph_node_type_t hgraph_core_var_str_out = {
	.name = HGRAPH_STR("core.out.var_str"),
	.label = HGRAPH_STR("String out"),
	.description = HGRAPH_STR("Output a single string. This can only be used programmatically."),
	.group = HGRAPH_STR("Core.Output"),
	.size = 0,
	.alignment = _Alignof(char),
	.attributes = HGRAPH_NODE_ATTRIBUTES(
		&hgraph_core_var_str_out_attr_value
	),
	.output_pins = HGRAPH_NODE_PINS(
		&hgraph_core_var_str_out_out
	),
	.execute = hgraph_core_var_str_out_execute,
};
