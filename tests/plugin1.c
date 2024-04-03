#include "plugin1.h"
#include "data.h"
#include "hgraph/plugin.h"

static void
plugin1_start_execute(const hgraph_node_api_t* api) {
	float attr = *(const float*)hgraph_node_input(api, &plugin1_start_attr_f32);
	hgraph_node_output(api, &plugin1_start_out_f32, &attr);
}

static void
plugin1_end_execute(const hgraph_node_api_t* api) {
	int32_t input = *(const int32_t*)hgraph_node_input(api, &plugin1_end_in_i32);
	int32_t* status = hgraph_node_allocate(api, HGRAPH_LIFETIME_EXECUTION, sizeof(int32_t));
	*status = input;

	hgraph_node_report_status(api, status);
}

const hgraph_node_type_t plugin1_start = {
	.name = HGRAPH_STR("start"),
	.attributes = HGRAPH_NODE_ATTRIBUTES(
		&plugin1_start_attr_f32
	),
	.output_pins = HGRAPH_NODE_PINS(
		&plugin1_start_out_f32
	),
	.execute = plugin1_start_execute,
};

const hgraph_attribute_description_t plugin1_start_attr_f32 = {
	.name = HGRAPH_STR("num_f32"),
	.data_type = &test_f32,
};

const hgraph_pin_description_t plugin1_start_out_f32 = {
	.name = HGRAPH_STR("num_out_f32"),
	.data_type = &test_f32,
};

const hgraph_node_type_t plugin1_end = {
	.name = HGRAPH_STR("end"),
	.input_pins = HGRAPH_NODE_PINS(
		&plugin1_end_in_i32
	),
	.execute = plugin1_end_execute,
};

const hgraph_pin_description_t plugin1_end_in_i32 = {
	.name = HGRAPH_STR("num_in_i32"),
	.data_type = &test_i32,
};

void
plugin1_entry(hgraph_plugin_api_t* api) {
	hgraph_plugin_register_node_type(api, &plugin1_start);
	hgraph_plugin_register_node_type(api, &plugin1_end);
}
