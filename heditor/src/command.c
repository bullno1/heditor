#include "command.h"
#include "pico_log.h"

#define MAX_COMMANDS 8

static int hed_cmd_write_pos = 0;
static int hed_cmd_read_pos = 0;
static hed_command_t hed_cmd_buf[MAX_COMMANDS];

void
hed_cmd(hed_command_t cmd) {
	if (hed_cmd_write_pos >= MAX_COMMANDS) {
		log_warn("Command buffer full");
		return;
	}

	hed_cmd_buf[hed_cmd_write_pos++] = cmd;
}

bool
hed_next_cmd(hed_command_t* cmd) {
	if (hed_cmd_read_pos >= hed_cmd_write_pos) {
		hed_cmd_read_pos = hed_cmd_write_pos = 0;
		return false;
	} else {
		*cmd = hed_cmd_buf[hed_cmd_read_pos++];
		return true;
	}
}
