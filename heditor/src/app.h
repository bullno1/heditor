#ifndef HEDITOR_APP_H
#define HEDITOR_APP_H

#include "path.h"
#include "hstring.h"

typedef struct {
	hed_str_t project_name;
	hed_path_t* project_root;

	int num_plugins;
	struct plugin_entry_s* plugin_entries;
} app_config_t;

struct plugin_entry_s {
	hed_str_t name;
	struct plugin_entry_s* next;
};

#endif
