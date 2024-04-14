#ifndef HEDITOR_COMMAND_H
#define HEDITOR_COMMAND_H

#include <stdbool.h>
#include <hgraph/runtime.h>
#include <cimgui.h>

#define HED_CMD(...) \
	hed_cmd((hed_command_t){ .type = __VA_ARGS__ })

typedef enum hed_command_type_e {
	HED_CMD_EXIT,
	HED_CMD_OPEN,
	HED_CMD_CREATE_NODE,
	HED_CMD_CREATE_EDGE,
} hed_command_type_t;

typedef struct hed_command_s {
	hed_command_type_t type;
	union {
		struct {
			const struct hgraph_node_type_s* node_type;
			ImVec2 pos;
		} create_node_args;

		struct {
			hgraph_index_t from_pin;
			hgraph_index_t to_pin;
		} create_edge_args;
	};
} hed_command_t;

void
hed_cmd(hed_command_t cmd);

bool
hed_next_cmd(hed_command_t* cmd);

#endif
