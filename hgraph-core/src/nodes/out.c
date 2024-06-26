#include "hgraph/plugin.h"
#include <hgraph/core.h>
#include <heditor/plugin.h>
#include "../reg.h"

// f32
static const hgraph_pin_description_t hgraph_core_f32_out_out = {
	.name = HGRAPH_STR("out"),
	.label = HGRAPH_STR("Output"),
	.data_type = &hgraph_core_f32,
};

const hgraph_attribute_description_t hgraph_core_f32_out_attr_value = {
	.name = HGRAPH_STR("value"),
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

HGRAPH_CORE_NODE(hgraph_core_f32_out) = {
	.name = HGRAPH_STR("core.out.f32"),
	.label = HGRAPH_STR("Float out"),
	.description = HGRAPH_STR("Output a single float"),
	.group = HGRAPH_STR("Core/Output"),
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
	.name = HGRAPH_STR("out"),
	.label = HGRAPH_STR("Output"),
	.data_type = &hgraph_core_i32,
};

const hgraph_attribute_description_t hgraph_core_i32_out_attr_value = {
	.name = HGRAPH_STR("value"),
	.label = HGRAPH_STR("Value"),
	.description = HGRAPH_STR("The value to output"),
	.data_type = &hgraph_core_i32,
};

static inline void
hgraph_core_i32_out_execute(const hgraph_node_api_t* api) {
	int32_t value = *(const int32_t*)hgraph_node_input(api, &hgraph_core_i32_out_attr_value);
	hgraph_node_output(api, &hgraph_core_i32_out_out, &value);
}

HGRAPH_CORE_NODE(hgraph_core_i32_out) = {
	.name = HGRAPH_STR("core.out.i32"),
	.label = HGRAPH_STR("Integer out"),
	.description = HGRAPH_STR("Output a single integer"),
	.group = HGRAPH_STR("Core/Output"),
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
	.name = HGRAPH_STR("out"),
	.label = HGRAPH_STR("Output"),
	.data_type = &hgraph_core_var_str,
};

const hgraph_attribute_description_t hgraph_core_fixed_str_out_attr_value = {
	.name = HGRAPH_STR("value"),
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
	hgraph_str_t out = hgraph_core_alloc_str_output(api, value->len, value->data);
	hgraph_node_output(api, &hgraph_core_fixed_str_out_out, &out);
}

HGRAPH_CORE_NODE(hgraph_core_fixed_str_out) = {
	.name = HGRAPH_STR("core.out.fixed_str"),
	.label = HGRAPH_STR("String out (fixed)"),
	.description = HGRAPH_STR("Output a single fixed-size string"),
	.group = HGRAPH_STR("Core/Output"),
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
	.name = HGRAPH_STR("out"),
	.label = HGRAPH_STR("Output"),
	.data_type = &hgraph_core_var_str,
};

const hgraph_attribute_description_t hgraph_core_var_str_out_attr_value = {
	.name = HGRAPH_STR("value"),
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
	hgraph_str_t out = hgraph_core_alloc_str_output(api, value.length, value.data);
	hgraph_node_output(api, &hgraph_core_var_str_out_out, &out);
}

HGRAPH_CORE_NODE(hgraph_core_var_str_out) = {
	.name = HGRAPH_STR("core.out.var_str"),
	.label = HGRAPH_STR("String out (variable)"),
	.description = HGRAPH_STR("Output a single variably-sized string.\nThis can only be used programmatically."),
	.group = HGRAPH_STR("Core/Output"),
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

// Path

static const hgraph_pin_description_t hgraph_core_path_out_out = {
	.name = HGRAPH_STR("out"),
	.label = HGRAPH_STR("Output"),
	.data_type = &hgraph_core_var_str,
};

static void
hgraph_core_path_out_attr_path_init(void* value) {
	hgraph_core_fixed_str_t* fixed_str = value;
	fixed_str->len = 2;
	memcpy(fixed_str->data, "./", sizeof("./"));
}

static void
hgraph_core_path_out_attr_path_render(void* value, void* gui) {
	hgraph_core_fixed_str_t* fixed_str = value;
	hed_editable_str_t gui_str = {
		.capacity = HGRAPH_CORE_FIXED_STR_LEN,
		.len = fixed_str->len,
		.chars = fixed_str->data,
	};
	bool changed = hed_gui_render_string_input(
		gui,
		&gui_str,
		&(hed_text_input_opts_t){ .type = HED_TEXT_INPUT_FILE_PICKER }
	);
	if (changed) {
		fixed_str->len = gui_str.len;
	}
}

const hgraph_attribute_description_t hgraph_core_path_out_attr_path = {
	.name = HGRAPH_STR("path"),
	.label = HGRAPH_STR("Path"),
	.description = HGRAPH_STR("The path"),
	.data_type = &hgraph_core_fixed_str,
	.init = hgraph_core_path_out_attr_path_init,
	.render = hgraph_core_path_out_attr_path_render,
};

static inline void
hgraph_core_path_out_execute(const hgraph_node_api_t* api) {
	const hgraph_str_t value = *(const hgraph_str_t*)hgraph_node_input(
		api,
		&hgraph_core_path_out_attr_path
	);
	hgraph_str_t out = hgraph_core_alloc_str_output(api, value.length, value.data);
	hgraph_node_output(api, &hgraph_core_path_out_out, &out);
}

HGRAPH_CORE_NODE(hgraph_core_path_out) = {
	.name = HGRAPH_STR("core.out.path"),
	.label = HGRAPH_STR("Path out"),
	.description = HGRAPH_STR("Output a path relative to the project root."),
	.group = HGRAPH_STR("Core/Output"),
	.size = 0,
	.alignment = _Alignof(char),
	.attributes = HGRAPH_NODE_ATTRIBUTES(
		&hgraph_core_path_out_attr_path
	),
	.output_pins = HGRAPH_NODE_PINS(
		&hgraph_core_path_out_out
	),
	.execute = hgraph_core_path_out_execute,
};
