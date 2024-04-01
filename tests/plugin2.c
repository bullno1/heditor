#include "plugin2.h"
#include "data.h"
#include "hgraph/plugin.h"
#include <stdbool.h>
#include <math.h>

static void
round_up_default(void* value) {
	*(bool*)value = true;
}

static void
plugin2_mid_execute(const hgraph_node_api_t* api) {
	mid_state_t* data = hgraph_node_data(api);
	float input = *(const float*)hgraph_node_input(api, &plugin2_mid_in_f32);
	bool round_up = *(const bool*)hgraph_node_input(api, &plugin2_mid_attr_round_up);
	int32_t result = (int32_t)(round_up ? ceilf(input) : floorf(input));

	hgraph_node_output(api, &plugin2_mid_out_i32, &result);

	++data->num_executions;
	mid_state_t* status = hgraph_node_allocate(api, HGRAPH_LIFETIME_EXECUTION, sizeof(mid_state_t));
	*status = *data;
	hgraph_node_report_status(api, status);
}

static void
plugin2_mid_transfer(void* dst, void* src) {
	mid_state_t* dst_state = dst;
	mid_state_t* src_state = src;
	*dst_state = *src_state;
}

const hgraph_node_type_t plugin2_mid = {
	.name = HGRAPH_STR("mid"),
	.size = sizeof(mid_state_t),
	.alignment = _Alignof(mid_state_t),
	.attributes = HGRAPH_NODE_ATTRIBUTES(
		&plugin2_mid_attr_round_up
	),
	.input_pins = HGRAPH_NODE_PINS(
		&plugin2_mid_in_f32
	),
	.output_pins = HGRAPH_NODE_PINS(
		&plugin2_mid_out_i32
	),
	.execute = plugin2_mid_execute,
	.transfer = plugin2_mid_transfer,
};

const hgraph_pin_description_t plugin2_mid_in_f32 = {
	.name = HGRAPH_STR("f32"),
	.data_type = &test_f32,
};

const hgraph_pin_description_t plugin2_mid_out_i32 = {
	.name = HGRAPH_STR("i32"),
	.data_type = &test_i32,
};

const hgraph_attribute_description_t plugin2_mid_attr_round_up = {
	.name = HGRAPH_STR("round_up"),
	.data_type = &test_bool,
	.init = round_up_default,
};

void
plugin2_entry(hgraph_plugin_api_t* api) {
	hgraph_plugin_register_node_type(api, &plugin2_mid);
}
