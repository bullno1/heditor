#include <hgraph/core.h>

// i32_to_f32
static void
hgraph_core_i32_to_f32_execute(const hgraph_node_api_t* api);

const hgraph_node_type_t hgraph_core_i32_to_f32 = {
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
