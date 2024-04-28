#ifndef HEDITOR_APP_H
#define HEDITOR_APP_H

#include "path.h"
#include "hstring.h"
#include <hgraph/runtime.h>

typedef struct {
	int max_documents;
} editor_config_t;

typedef struct {
	hed_str_t project_name;
	hed_path_t* project_root;

	int num_plugins;
	struct plugin_entry_s* plugin_entries;

	hgraph_registry_config_t registry_config;
	hgraph_config_t graph_config;
	hgraph_pipeline_config_t pipeline_config;

	editor_config_t editor_config;
} app_config_t;

struct plugin_entry_s {
	hed_str_t name;
	struct plugin_entry_s* next;
};

static const editor_config_t DEFAULT_EDITOR_CONFIG = {
	.max_documents = 8,
};

extern hed_path_t* project_root;

#endif
