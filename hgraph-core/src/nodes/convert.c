#include <hgraph/core.h>
#include <heditor/plugin.h>
#include <math.h>
#include <stdio.h>
#include "../reg.h"

// i32_to_f32
static void
hgraph_core_i32_to_f32_execute(const hgraph_node_api_t* api);

HGRAPH_CORE_NODE(hgraph_core_i32_to_f32) = {
	.name = HGRAPH_STR("core.convert.i32_to_f32"),
	.label = HGRAPH_STR("i32 to f32"),
	.description = HGRAPH_STR("Convert i32 to f32"),
	.group = HGRAPH_STR("Core/Convert"),
	.size = 0,
	.alignment = _Alignof(char),
	.input_pins = HGRAPH_NODE_PINS(
		&(hgraph_pin_description_t){
			.name = HGRAPH_STR("in"),
			.label = HGRAPH_STR("Input"),
			.data_type = &hgraph_core_i32,
		}
	),
	.output_pins = HGRAPH_NODE_PINS(
		&(hgraph_pin_description_t){
			.name = HGRAPH_STR("out"),
			.label = HGRAPH_STR("Output"),
			.data_type = &hgraph_core_f32,
		}
	),
	.execute = hgraph_core_i32_to_f32_execute,
};

static void
hgraph_core_i32_to_f32_execute(const hgraph_node_api_t* api) {
	int32_t input = *(const int32_t*)hgraph_node_input(
		api,
		hgraph_core_i32_to_f32.input_pins[0]
	);
	float output = (float)input;
	hgraph_node_output(api, hgraph_core_i32_to_f32.output_pins[0], &output);
}

// f32_to_i32
static void
hgraph_core_f32_to_i32_execute(const hgraph_node_api_t* api);

static void
hgraph_core_f32_to_i32_attr_rounding_mode_render(void* value, void* gui);

const hgraph_attribute_description_t hgraph_core_f32_to_i32_attr_rounding_mode = {
	.name = HGRAPH_STR("rounding_mode"),
	.label = HGRAPH_STR("Rounding mode"),
	.data_type = &hgraph_core_i32,
	.render = hgraph_core_f32_to_i32_attr_rounding_mode_render,
};

HGRAPH_CORE_NODE(hgraph_core_f32_to_i32) = {
	.name = HGRAPH_STR("core.convert.f32_to_i32"),
	.label = HGRAPH_STR("f32 to i32"),
	.description = HGRAPH_STR("Convert f32 to i32"),
	.group = HGRAPH_STR("Core/Convert"),
	.size = 0,
	.alignment = _Alignof(char),
	.attributes = HGRAPH_NODE_ATTRIBUTES(
		&hgraph_core_f32_to_i32_attr_rounding_mode
	),
	.input_pins = HGRAPH_NODE_PINS(
		&(hgraph_pin_description_t){
			.name = HGRAPH_STR("in"),
			.label = HGRAPH_STR("Input"),
			.data_type = &hgraph_core_f32,
		}
	),
	.output_pins = HGRAPH_NODE_PINS(
		&(hgraph_pin_description_t){
			.name = HGRAPH_STR("out"),
			.label = HGRAPH_STR("Output"),
			.data_type = &hgraph_core_i32,
		}
	),
	.execute = hgraph_core_f32_to_i32_execute,
};

static void
hgraph_core_f32_to_i32_execute(const hgraph_node_api_t* api) {
	int32_t rounding_mode = *(const int32_t*)hgraph_node_input(
		api,
		&hgraph_core_f32_to_i32_attr_rounding_mode
	);
	float input = *(const float*)hgraph_node_input(
		api,
		hgraph_core_f32_to_i32.input_pins[0]
	);

	int32_t output;
	switch ((hgraph_core_rounding_mode_t)rounding_mode) {
		case HGRAPH_CORE_ROUND_NEAREST:
			output = (int32_t)roundf(input);
			break;
		case HGRAPH_CORE_ROUND_FLOOR:
			output = (int32_t)floorf(input);
			break;
		case HGRAPH_CORE_ROUND_CEIL:
			output = (int32_t)ceilf(input);
			break;
		default:
			output = (int32_t)input;
			break;
	}

	hgraph_node_output(api, hgraph_core_f32_to_i32.output_pins[0], &output);
}

static void
hgraph_core_f32_to_i32_attr_rounding_mode_render(void* value, void* gui) {
	hed_gui_render_enum(gui, value, 3, (const char*[]){
		"Nearest",
		"Floor",
		"Ceiling"
	});
}

// i32_to_str
static void
hgraph_core_i32_to_str_execute(const hgraph_node_api_t* api);

HGRAPH_CORE_NODE(hgraph_core_i32_to_str) = {
	.name = HGRAPH_STR("core.convert.i32_to_str"),
	.label = HGRAPH_STR("i32 to str"),
	.description = HGRAPH_STR("Convert i32 to string"),
	.group = HGRAPH_STR("Core/Convert"),
	.size = 0,
	.alignment = _Alignof(char),
	.input_pins = HGRAPH_NODE_PINS(
		&(hgraph_pin_description_t){
			.name = HGRAPH_STR("in"),
			.label = HGRAPH_STR("Input"),
			.data_type = &hgraph_core_i32,
		}
	),
	.output_pins = HGRAPH_NODE_PINS(
		&(hgraph_pin_description_t){
			.name = HGRAPH_STR("out"),
			.label = HGRAPH_STR("Output"),
			.data_type = &hgraph_core_var_str,
		}
	),
	.execute = hgraph_core_i32_to_str_execute,
};

static void
hgraph_core_i32_to_str_execute(const hgraph_node_api_t* api) {
	char buf[32];
	int32_t input = *(const int32_t*)hgraph_node_input(
		api,
		hgraph_core_i32_to_str.input_pins[0]
	);
	int len = snprintf(buf, sizeof(buf), "%d", input);
	hgraph_str_t out = hgraph_core_alloc_str_output(api, (size_t)len, buf);
	hgraph_node_output(api, hgraph_core_i32_to_str.output_pins[0], &out);
}

// f32_to_str
static void
hgraph_core_f32_to_str_execute(const hgraph_node_api_t* api);

HGRAPH_CORE_NODE(hgraph_core_f32_to_str) = {
	.name = HGRAPH_STR("core.convert.f32_to_str"),
	.label = HGRAPH_STR("f32 to str"),
	.description = HGRAPH_STR("Convert f32 to string"),
	.group = HGRAPH_STR("Core/Convert"),
	.size = 0,
	.alignment = _Alignof(char),
	.input_pins = HGRAPH_NODE_PINS(
		&(hgraph_pin_description_t){
			.name = HGRAPH_STR("in"),
			.label = HGRAPH_STR("Input"),
			.data_type = &hgraph_core_f32,
		}
	),
	.output_pins = HGRAPH_NODE_PINS(
		&(hgraph_pin_description_t){
			.name = HGRAPH_STR("out"),
			.label = HGRAPH_STR("Output"),
			.data_type = &hgraph_core_var_str,
		}
	),
	.execute = hgraph_core_f32_to_str_execute,
};

static void
hgraph_core_f32_to_str_execute(const hgraph_node_api_t* api) {
	char buf[32];
	float input = *(const float*)hgraph_node_input(
		api,
		hgraph_core_f32_to_str.input_pins[0]
	);
	int len = snprintf(buf, sizeof(buf), "%f", input);
	hgraph_str_t out = hgraph_core_alloc_str_output(api, (size_t)len, buf);
	hgraph_node_output(api, hgraph_core_f32_to_str.output_pins[0], &out);
}
