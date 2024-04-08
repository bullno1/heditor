#ifndef HEDITOR_COMMAND_H
#define HEDITOR_COMMAND_H

#include <stdbool.h>

#define HEDITOR_CMD(...) \
	heditor_cmd((heditor_command_t){ .type = __VA_ARGS__ })

typedef enum heditor_command_type_e {
	HEDITOR_CMD_EXIT,
	HEDITOR_CMD_OPEN,
} heditor_command_type_t;

typedef struct heditor_command_s {
	heditor_command_type_t type;
} heditor_command_t;

void
heditor_cmd(heditor_command_t cmd);

bool
heditor_next_cmd(heditor_command_t* cmd);

#endif
