#include "app.h"
#include "path.h"
#include "pico_log.h"
#include "hstring.h"
#include "allocator/allocator.h"
#include "allocator/arena.h"
#include <stdio.h>
#include <string.h>
#include <ini.h>

typedef struct {
	hed_allocator_t* alloc;
	app_config_t* config;
} config_load_ctx_t;

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
		}
	} else {
		return 0;
	}

	return 1;
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
