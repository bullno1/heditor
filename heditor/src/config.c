#include "app.h"
#include "path.h"
#include "pico_log.h"
#include "hstring.h"
#include "allocator/allocator.h"
#include "allocator/arena.h"
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ini.h>

typedef struct {
	hed_allocator_t* alloc;
	app_config_t* config;
} config_load_ctx_t;

static bool
parse_size(const char* str, hgraph_index_t* out) {
	errno = 0;
	char* end;
	long result = strtol(str, &end, 10);

	if (
		errno != 0
		|| end != str + strlen(str)
		|| result < 0
		|| result > INT32_MAX
	) {
		return false;
	} else {
		*out = result;
		return true;
	}
}

static int
parse_app_config(
	void* userdata,
	const char* section,
	const char* name,
	const char* value
) {
	config_load_ctx_t* ctx = userdata;

	if (strcmp(section, "project") == 0) {
		if (strcmp(name, "name") == 0) {
			ctx->config->project_name = hed_strcpy(
				ctx->alloc,
				hed_str_from_cstr(value)
			);
			return 1;
		} else if (strcmp(name, "plugins") == 0) {
			++ctx->config->num_plugins;
			struct plugin_entry_s* entry = hed_malloc(
				sizeof(struct plugin_entry_s),
				ctx->alloc
			);
			entry->name = hed_strcpy(
				ctx->alloc,
				hed_str_from_cstr(value)
			);
			entry->next = ctx->config->plugin_entries;
			ctx->config->plugin_entries = entry;
			return 1;
		} else {
			return 0;
		}
	} else if (strcmp(section, "registry") == 0) {
		if (strcmp(name, "max_data_types") == 0) {
			return parse_size(value, &ctx->config->registry_max_data_types);
		} else if (strcmp(name, "max_node_types") == 0) {
			return parse_size(value, &ctx->config->registry_max_node_types);
		} else {
			return 0;
		}
	} else if (strcmp(section, "graph") == 0) {
		if (strcmp(name, "max_nodes") == 0) {
			return parse_size(value, &ctx->config->graph_max_nodes);
		} else if (strcmp(name, "max_name_length") == 0) {
			return parse_size(value, &ctx->config->graph_max_name_length);
		} else {
			return 0;
		}
	} else if (strcmp(section, "pipeline") == 0) {
		if (strcmp(name, "max_scratch_memory") == 0) {
			return parse_size(value, &ctx->config->pipeline_max_scratch_memory);
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

app_config_t*
load_app_config(hed_arena_t* arena) {
	hed_allocator_t* alloc = hed_arena_as_allocator(arena);
	hed_arena_checkpoint_t checkpoint = hed_arena_begin(arena);

	for (
		hed_path_t* path = hed_path_current(alloc);
		path != NULL;
		hed_arena_end(arena, checkpoint),
		path = hed_path_dirname(alloc, path)
	) {
		hed_path_t* config_path = hed_path_join(
			alloc,
			path,
			(const char*[]){ "heditor.ini", NULL }
		);

		const char* str_path = hed_path_as_str(config_path);
		FILE* config_file = fopen(str_path, "rb");
		if (config_file != NULL) {
			log_info("Found config at: %s", str_path);

			app_config_t* config = hed_arena_alloc(
				arena, sizeof(app_config_t), _Alignof(app_config_t)
			);
			*config = (app_config_t){
				.project_root = path,
				.registry_max_data_types = 256,
				.registry_max_node_types = 256,
				.graph_max_nodes = 63,
				.graph_max_name_length = 31,
				.pipeline_max_scratch_memory = 33554432,
			};
			config_load_ctx_t ctx = {
				.alloc = alloc,
				.config = config,
			};
			int result = ini_parse_file(config_file, parse_app_config, &ctx);
			fclose(config_file);

			if (result != 0) {
				log_error("Config error at line: %d", result);
				return NULL;
			} else {
				return config;
			}
		}

	}

	log_error("Could not find config file");
	hed_arena_end(arena, checkpoint);
	return NULL;
}
