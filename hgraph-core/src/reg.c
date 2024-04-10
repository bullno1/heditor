#define REMODULE_PLUGIN_IMPLEMENTATION
#include "hgraph/plugin.h"
#include <hgraph/core.h>
#include <remodule.h>
#include <autolist.h>

AUTOLIST_DECLARE(hgraph_core_nodes)

void
hgraph_core_register(const hgraph_plugin_api_t* api) {
	for (
		const autolist_entry_t* const* itr = AUTOLIST_BEGIN(hgraph_core_nodes);
		itr != AUTOLIST_END(hgraph_core_nodes);
		++itr
	) {
		if (*itr == NULL) { continue; }

		hgraph_plugin_register_node_type(api, (*itr)->value_addr);
	}
}

void
remodule_entry(remodule_op_t op, void* userdata) {
	if (op == REMODULE_OP_LOAD || op == REMODULE_OP_AFTER_RELOAD) {
		hgraph_core_register(userdata);
	}
}
