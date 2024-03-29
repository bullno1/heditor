#include "plugin2.h"
#include "data.h"

const hgraph_node_type_t plugin2_mid = {
	.name = HGRAPH_STR("mid"),
	.input_pins = HGRAPH_NODE_PINS(
		&plugin2_mid_in_f32
	),
	.output_pins = HGRAPH_NODE_PINS(
		&plugin2_mid_out_i32
	),
};

const hgraph_pin_description_t plugin2_mid_in_f32 = {
	.name = HGRAPH_STR("f32"),
	.data_type = &test_f32,
};

const hgraph_pin_description_t plugin2_mid_out_i32 = {
	.name = HGRAPH_STR("i32"),
	.data_type = &test_i32,
};

void
plugin2_entry(hgraph_plugin_api_t* api) {
	hgraph_plugin_register_node_type(api, &plugin2_mid);
}
