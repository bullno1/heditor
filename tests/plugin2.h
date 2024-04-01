#ifndef HGRAPH_TESTS_PLUGIN2_H
#define HGRAPH_TESTS_PLUGIN2_H

#include <hgraph/plugin.h>

extern const hgraph_node_type_t plugin2_mid;
extern const hgraph_pin_description_t plugin2_mid_in_f32;
extern const hgraph_pin_description_t plugin2_mid_out_i32;
extern const hgraph_attribute_description_t plugin2_mid_attr_round_up;

typedef struct {
	int num_executions;
} mid_state_t;

void
plugin2_entry(hgraph_plugin_api_t* api);

#endif
