#ifndef HGRAPH_TESTS_PLUGIN1_H
#define HGRAPH_TESTS_PLUGIN1_H

#include <hgraph/plugin.h>

extern const hgraph_node_type_t plugin1_start;
extern const hgraph_pin_description_t plugin1_start_out_f32;

extern const hgraph_node_type_t plugin1_end;
extern const hgraph_pin_description_t plugin1_end_in_i32;

void
plugin1_entry(hgraph_plugin_api_t* api);

#endif
