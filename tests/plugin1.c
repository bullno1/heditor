#include "plugin1.h"
#include "data.h"
#include "hgraph/plugin.h"

const hgraph_node_type_t plugin1_start = {
	.name = HGRAPH_STR("start"),
	.output_pins = HGRAPH_NODE_PINS(
		&plugin1_start_out_f32
	),
};

const hgraph_pin_description_t plugin1_start_out_f32 = {
	.name = HGRAPH_STR("f32"),
	.data_type = &test_f32,
};

const hgraph_node_type_t plugin1_end = {
	.name = HGRAPH_STR("end"),
	.output_pins = HGRAPH_NODE_PINS(
		&plugin1_end_in_i32
	),
};

const hgraph_pin_description_t plugin1_end_in_i32 = {
	.name = HGRAPH_STR("i32"),
	.data_type = &test_i32,
};

void
plugin1_entry(hgraph_plugin_api_t* api) {
	hgraph_plugin_register_node_type(api, &plugin1_start);
	hgraph_plugin_register_node_type(api, &plugin1_end);
}
