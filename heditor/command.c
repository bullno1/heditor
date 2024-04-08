#include "command.h"
#include "pico_log.h"

#define MAX_COMMANDS 8

static int heditor_cmd_write_pos = 0;
static int heditor_cmd_read_pos = 0;
static heditor_command_t heditor_cmd_buf[MAX_COMMANDS];

void
heditor_cmd(heditor_command_t cmd) {
	if (heditor_cmd_write_pos >= MAX_COMMANDS) {
		log_warn("Command buffer full");
		return;
	}

	heditor_cmd_buf[heditor_cmd_write_pos++] = cmd;
}

bool
heditor_next_cmd(heditor_command_t* cmd) {
	if (heditor_cmd_read_pos >= heditor_cmd_write_pos) {
		heditor_cmd_read_pos = heditor_cmd_write_pos = 0;
		return false;
	} else {
		*cmd = heditor_cmd_buf[heditor_cmd_read_pos++];
		return true;
	}
}
