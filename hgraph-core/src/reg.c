#define REMODULE_PLUGIN_IMPLEMENTATION
#include "hgraph/plugin.h"
#include <hgraph/core.h>
#include <remodule.h>

void
hgraph_core_register(const hgraph_plugin_api_t* api) {
	hgraph_plugin_register_node_type(api, &hgraph_core_f32_out);
	hgraph_plugin_register_node_type(api, &hgraph_core_i32_out);
	hgraph_plugin_register_node_type(api, &hgraph_core_fixed_str_out);
	hgraph_plugin_register_node_type(api, &hgraph_core_var_str_out);
}

void
remodule_entry(remodule_op_t op, void* userdata) {
	if (op == REMODULE_OP_LOAD || op == REMODULE_OP_AFTER_RELOAD) {
		hgraph_core_register(userdata);
	}
}
