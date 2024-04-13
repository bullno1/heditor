#ifndef HEDITOR_APP_H
#define HEDITOR_APP_H

#include "path.h"
#include "hstring.h"
#include <hgraph/runtime.h>

typedef struct {
	hed_str_t project_name;
	hed_path_t* project_root;

	int num_plugins;
	struct plugin_entry_s* plugin_entries;

	hgraph_index_t registry_max_data_types;
	hgraph_index_t registry_max_node_types;
	hgraph_index_t graph_max_nodes;
	hgraph_index_t graph_max_name_length;
	hgraph_index_t pipeline_max_scratch_memory;
} app_config_t;

struct plugin_entry_s {
	hed_str_t name;
	struct plugin_entry_s* next;
};

#endif
