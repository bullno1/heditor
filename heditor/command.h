#ifndef HEDITOR_COMMAND_H
#define HEDITOR_COMMAND_H

#include <stdbool.h>

#define HED_CMD(...) \
	hed_cmd((hed_command_t){ .type = __VA_ARGS__ })

typedef enum hed_command_type_e {
	HED_CMD_EXIT,
	HED_CMD_OPEN,
} hed_command_type_t;

typedef struct hed_command_s {
	hed_command_type_t type;
} hed_command_t;

void
hed_cmd(hed_command_t cmd);

bool
hed_next_cmd(hed_command_t* cmd);

#endif
