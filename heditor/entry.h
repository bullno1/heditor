#ifndef HEDITOR_ENTRY_H
#define HEDITOR_ENTRY_H

#include <sokol_app.h>

typedef struct entry_args_s {
	// Input
	int argc;
	char** argv;
	sapp_logger logger;

	// Output
	sapp_desc app;
} entry_args_t;

#endif
