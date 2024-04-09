#define REMODULE_PLUGIN_IMPLEMENTATION
#include <remodule.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <util/sokol_imgui.h>
#include "sfd/sfd.h"
#include "pico_log.h"
#include "command.h"
#include "detect_debugger.h"
#include <string.h>
#include <errno.h>

#ifdef NDEBUG
REMODULE_VAR(bool, heditor_debug) = false;
#else
REMODULE_VAR(bool, heditor_debug) = true;
#endif

typedef struct {
	sg_pass_action pass_action;
} app_state_t;

REMODULE_VAR(app_state_t, state) = { 0 };
static sapp_logger logger = { 0 };

static
void init(void) {
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger = {
			.func = logger.func,
			.user_data = logger.user_data,
		},
	});

	simgui_setup(&(simgui_desc_t){
		.logger = {
			.func = logger.func,
			.user_data = logger.user_data,
		},
	});

	state.pass_action = (sg_pass_action) {
		.colors[0] = {
			.load_action = SG_LOADACTION_CLEAR,
			.clear_value = { 0.5f, 0.5f, 0.5f, 1.0 }
		},
	};

	heditor_debug = heditor_debug || is_debugger_attached();
	igGetIO()->ConfigDebugIsDebuggerPresent = heditor_debug;
}

static void
event(const sapp_event* ev) {
	simgui_handle_event(ev);
}

static void
frame(void) {
	// GUI
	simgui_new_frame(&(simgui_frame_desc_t){
		.width = sapp_width(),
		.height = sapp_height(),
		.delta_time = sapp_frame_duration(),
		.dpi_scale = sapp_dpi_scale(),
	});

	static bool show_imgui_demo = false;
	if (igBeginMainMenuBar()) {
		if (igBeginMenu("File", true)) {
			if (igMenuItem_Bool("New", NULL, false, true)) {
				log_trace("New");
			}

			if (igMenuItem_Bool("Open", "Ctrl+O", false, true)) {
				HEDITOR_CMD(HEDITOR_CMD_OPEN);
			}

			if (igMenuItem_Bool("Save", NULL, false, true)) {
				log_trace("Save");
			}

			igSeparator();

			if (igMenuItem_Bool("Exit", NULL, false, true)) {
				HEDITOR_CMD(HEDITOR_CMD_EXIT);
			}

			igEndMenu();
		}

		if (heditor_debug && igBeginMenu("Debug", true)) {
			if (igMenuItem_Bool("Dear ImGui", NULL, show_imgui_demo, true)) {
				show_imgui_demo = !show_imgui_demo;
			}
			igEndMenu();
		}

		igEndMainMenuBar();
	}

	// Hot keys
	if (igIsKeyChordPressed_Nil(ImGuiKey_O | ImGuiMod_Ctrl)) {
		HEDITOR_CMD(HEDITOR_CMD_OPEN);
	}

	// Handle GUI commands
	if (show_imgui_demo) {
		igShowDemoWindow(&show_imgui_demo);
	}

	heditor_command_t cmd;
	while (heditor_next_cmd(&cmd)) {
		switch (cmd.type) {
			case HEDITOR_CMD_EXIT:
				sapp_request_quit();
				break;
			case HEDITOR_CMD_OPEN:
				{
					errno = 0;
					const char* path = sfd_open_dialog(&(sfd_Options){
						.title = "Open a graph file",
						.filter_name = "Graph file",
						.filter = "*.hed",
					});
					log_trace("File: %s", path != NULL ? path : "<no file>");

					if (path == NULL) {
						const char* error = sfd_get_error();
						if (error) {
							log_error("%s (%d)", error, strerror(errno));
						}
					}
				}
				break;
		}
	}

	// Render
	sg_begin_pass(&(sg_pass){
		.action = state.pass_action,
		.swapchain = sglue_swapchain()
	});
	simgui_render();
	sg_end_pass();
	sg_commit();
}

static void
cleanup(void) {
	simgui_shutdown();
	sg_shutdown();
}

void
remodule_entry(remodule_op_t op, void* userdata) {
	sapp_desc* app = userdata;
	if (op == REMODULE_OP_LOAD || op == REMODULE_OP_AFTER_RELOAD) {
		logger = app->logger;
		*app = (sapp_desc){
			.init_cb = init,
			.cleanup_cb = cleanup,
			.event_cb = event,
			.frame_cb = frame,
			.window_title = "heditor",
			.width = 1280,
			.height = 720,
			.icon.sokol_default = true,
		};
	}
}
