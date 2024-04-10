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
#include "resources.h"
#include <xincbin.h>
#include <string.h>
#include <errno.h>
#include <qoi.h>
#include <entry.h>

#ifdef NDEBUG
REMODULE_VAR(bool, heditor_debug) = false;
#else
REMODULE_VAR(bool, heditor_debug) = true;
#endif

REMODULE_VAR(bool, show_imgui_demo) = false;

static
void init(void* userdata) {
	entry_args_t* args = userdata;
	free((void*)args->app.icon.images[0].pixels.ptr);

	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger = {
			.func = args->logger.func,
			.user_data = args->logger.user_data,
		},
	});

	simgui_setup(&(simgui_desc_t){
		.logger = {
			.func = args->logger.func,
			.user_data = args->logger.user_data,
		},
	});

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
		.action =  {
			.colors[0] = {
				.load_action = SG_LOADACTION_CLEAR,
				.clear_value = { 0.5f, 0.5f, 0.5f, 1.0 }
			},
		},
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

static sapp_icon_desc
load_app_icon(void) {
	xincbin_data_t qoi_icon = XINCBIN_GET(icon);
	struct qoidecoder decoder = qoidecoder(qoi_icon.data, (int)qoi_icon.size);
	if (decoder.error) {
		return (sapp_icon_desc){ .sokol_default = true };
	}

	int num_pixels = decoder.count;
	size_t icon_size = sizeof(unsigned) * num_pixels;
	unsigned* pixels = malloc(icon_size);
	for (int i = 0; i < num_pixels; ++i) {
		pixels[i] = qoidecode(&decoder);
	}

	if (decoder.error) {
		free(pixels);
		return (sapp_icon_desc){ .sokol_default = true };
	} else {
		return (sapp_icon_desc){
			.images = {
				{
					.width = decoder.width,
					.height = decoder.height,
					.pixels = {
						.ptr = pixels,
						.size = icon_size,
					},
				},
			},
		};
	}
}

void
remodule_entry(remodule_op_t op, void* userdata) {
	entry_args_t* args = userdata;
	if (op == REMODULE_OP_LOAD || op == REMODULE_OP_AFTER_RELOAD) {
		args->app = (sapp_desc){
			.init_userdata_cb = init,
			.cleanup_cb = cleanup,
			.event_cb = event,
			.frame_cb = frame,
			.window_title = "heditor",
			.width = 1280,
			.height = 720,
			.icon = op == REMODULE_OP_LOAD
				? load_app_icon()
				: (sapp_icon_desc){ .sokol_default = true },
			.user_data = userdata,
		};
	}
}
