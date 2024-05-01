#ifndef HEDITOR_ENTRY_H
#define HEDITOR_ENTRY_H

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <util/sokol_imgui.h>
#include "allocator/allocator.h"

typedef struct entry_args_s {
	// Input
	int argc;
	char** argv;
	hed_allocator_t* allocator;
	sapp_logger sokol_logger;
	sapp_allocator sokol_allocator;

	// Output
	sapp_desc app;

	// Status
	bool reload_blocked;
} entry_args_t;

#endif
