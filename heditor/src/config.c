#include "allocator/arena.h"
#include "path.h"
#include "pico_log.h"
#include <stdio.h>

void
load_app_config(hed_arena_t* arena) {
	hed_allocator_t* alloc = hed_arena_as_allocator(arena);
	hed_path_t* path = hed_path_current(alloc);

	while (path != NULL) {
		HED_WITH_ARENA(arena) {
			hed_path_t* config_path = hed_path_join(
				alloc,
				path,
				(const char*[]){ "heditor.ini", NULL }
			);

			const char* str_path = hed_path_as_str(config_path);
			FILE* config_file = fopen(str_path, "rb");
			if (config_file != NULL) {
				path = NULL;
				log_info("Found config at: %s", str_path);
				fclose(config_file);
				break;
			}
		}

		path = hed_path_dirname(alloc, path);
	}
}
