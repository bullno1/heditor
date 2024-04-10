#define REMODULE_PLUGIN_IMPLEMENTATION
#include "hgraph/plugin.h"
#include <hgraph/core.h>
#include <remodule.h>
#include <autolist.h>

AUTOLIST_DECLARE(hgraph_core_nodes)

void
hgraph_core_register(const hgraph_plugin_api_t* api) {
	AUTOLIST_FOREACH(itr, hgraph_core_nodes) {
		hgraph_plugin_register_node_type(api, (*itr)->value_addr);
	}
}

void
remodule_entry(remodule_op_t op, void* userdata) {
	if (op == REMODULE_OP_LOAD || op == REMODULE_OP_AFTER_RELOAD) {
		hgraph_core_register(userdata);
	}
}
