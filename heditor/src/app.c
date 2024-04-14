#include "hstring.h"
#include <remodule_monitor.h>
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
#include "entry.h"
#include "app.h"
#include "allocator/arena.h"
#include "cnode-editor.h"
#include <string.h>
#include <errno.h>

#define PLUGIN_PREFIX "lib"
#ifndef NDEBUG
#	define PLUGIN_SUFFIX "_d"
#else
#	define PLUGIN_SUFFIX ""
#endif

#ifdef NDEBUG
REMODULE_VAR(bool, hed_debug) = false;
#else
REMODULE_VAR(bool, hed_debug) = true;
#endif

REMODULE_VAR(bool, show_imgui_demo) = false;
REMODULE_VAR(hed_arena_t, frame_arena) = { 0 };
REMODULE_VAR(int, num_plugins) = 0;
REMODULE_VAR(remodule_t**, plugins) = NULL;
REMODULE_VAR(remodule_monitor_t**, plugin_monitors) = NULL;
REMODULE_VAR(hgraph_registry_builder_t*, registry_builder) = NULL;
REMODULE_VAR(size_t, current_registry_size) = 0;
REMODULE_VAR(hgraph_registry_t*, current_registry) = 0;
REMODULE_VAR(neEditorContext_t*, node_editor) = 0;

extern sapp_icon_desc
load_app_icon(hed_arena_t* arena);

extern app_config_t*
load_app_config(hed_arena_t* arena);

static
void init(void* userdata) {
	entry_args_t* args = userdata;

	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger = {
			.func = args->sokol_logger.func,
			.user_data = args->sokol_logger.user_data,
		},
		.allocator = {
			.alloc_fn = args->sokol_allocator.alloc_fn,
			.free_fn = args->sokol_allocator.free_fn,
			.user_data = args->sokol_allocator.user_data,
		},
	});

	simgui_setup(&(simgui_desc_t){
		.logger = {
			.func = args->sokol_logger.func,
			.user_data = args->sokol_logger.user_data,
		},
		.allocator = {
			.alloc_fn = args->sokol_allocator.alloc_fn,
			.free_fn = args->sokol_allocator.free_fn,
			.user_data = args->sokol_allocator.user_data,
		},
	});

	hed_debug = hed_debug || is_debugger_attached();
	igGetIO()->ConfigDebugIsDebuggerPresent = hed_debug;

	node_editor = neCreateEditor();
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

	// Main menu
	if (igBeginMainMenuBar()) {
		if (igBeginMenu("File", true)) {
			if (igMenuItem_Bool("New", NULL, false, true)) {
				log_trace("New");
			}

			if (igMenuItem_Bool("Open", "Ctrl+O", false, true)) {
				HED_CMD(HED_CMD_OPEN);
			}

			if (igMenuItem_Bool("Save", NULL, false, true)) {
				log_trace("Save");
			}

			igSeparator();

			if (igMenuItem_Bool("Exit", NULL, false, true)) {
				HED_CMD(HED_CMD_EXIT);
			}

			igEndMenu();
		}

		if (hed_debug && igBeginMenu("Debug", true)) {
			if (igMenuItem_Bool("Dear ImGui", NULL, show_imgui_demo, true)) {
				show_imgui_demo = !show_imgui_demo;
			}
			igEndMenu();
		}

		igEndMainMenuBar();
	}

	// Main window
	const ImGuiWindowFlags main_window_flags =
		ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
	ImGuiViewport* viewport = igGetMainViewport();
	igSetNextWindowPos(viewport->WorkPos, ImGuiCond_None, (ImVec2){ 0 });
	igSetNextWindowSize(viewport->WorkSize, ImGuiCond_None);
	igBegin("Main", NULL, main_window_flags);
	{
		neBegin("Editor", (ImVec2){ 0 });
		{
		}
		neEnd();
	}
	igEnd();

	// Hot keys
	if (igIsKeyChordPressed_Nil(ImGuiKey_O | ImGuiMod_Ctrl)) {
		HED_CMD(HED_CMD_OPEN);
	}

	// Handle GUI commands
	if (show_imgui_demo) {
		igShowDemoWindow(&show_imgui_demo);
	}

	hed_command_t cmd;
	while (hed_next_cmd(&cmd)) {
		switch (cmd.type) {
			case HED_CMD_EXIT:
				sapp_request_quit();
				break;
			case HED_CMD_OPEN:
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

	hed_arena_reset(&frame_arena);
}

static void
cleanup(void* userdata) {
	entry_args_t* args = userdata;

	neDestroyEditor(node_editor);

	hed_free(current_registry, args->allocator);

	for (int i = num_plugins - 1; i >= 0; --i) {
		remodule_unmonitor(plugin_monitors[i]);
		remodule_unload(plugins[i]);
	}
	hed_free(plugin_monitors, args->allocator);
	hed_free(plugins, args->allocator);

	hed_free(registry_builder, args->allocator);

	simgui_shutdown();
	sg_shutdown();

	hed_arena_cleanup(&frame_arena);
}

void
remodule_entry(remodule_op_t op, void* userdata) {
	entry_args_t* args = userdata;
	if (op == REMODULE_OP_LOAD) {
		hed_arena_init(&frame_arena, 4 * 1024 * 1024, args->allocator);

		// TODO: config reload
		HED_WITH_ARENA(&frame_arena) {
			app_config_t* config = load_app_config(&frame_arena);
			if (config == NULL) {
				log_warn("Could not find config");
			} else {
				log_debug("Project root: %s", hed_path_as_str(config->project_root));
				log_debug("Project name: %s", config->project_name.data);

				size_t registry_builder_size = hgraph_registry_builder_init(
					NULL, &config->registry_config
				);
				log_debug("Registry builder size: %zu", registry_builder_size);
				registry_builder = hed_malloc(registry_builder_size, args->allocator);
				hgraph_registry_builder_init(
					registry_builder, &config->registry_config
				);
				hgraph_plugin_api_t* plugin_api = hgraph_registry_builder_as_plugin_api(
					registry_builder
				);
				plugins = hed_malloc(
					sizeof(remodule_t*) * config->num_plugins,
					args->allocator
				);
				plugin_monitors = hed_malloc(
					sizeof(remodule_monitor_t*) * config->num_plugins,
					args->allocator
				);
				for (
					struct plugin_entry_s* itr = config->plugin_entries;
					itr != NULL;
					itr = itr->next
				) {
					// TODO: if the path starts with `.`, it is relative to the
					// root.
					// Otherwise, let the dynamic loader resolve it.
					hed_str_t module_name = hed_str_format(
						hed_arena_as_allocator(&frame_arena),
						"%s%s%s%s",
						PLUGIN_PREFIX,
						itr->name.data,
						PLUGIN_SUFFIX, REMODULE_DYNLIB_EXT
					);
					log_debug("Loading plugin: %s", module_name.data);
					plugins[num_plugins] = remodule_load(
						module_name.data, plugin_api
					);
					plugin_monitors[num_plugins] = remodule_monitor(
						plugins[num_plugins]
					);
					++num_plugins;
				}
			}
		}

		if (registry_builder != NULL) {
			current_registry_size = hgraph_registry_init(
				NULL, registry_builder
			);
			log_debug("Registry size: %zu", current_registry_size);
			current_registry = hed_malloc(current_registry_size, args->allocator);
			hgraph_registry_init(current_registry, registry_builder);
			hgraph_registry_info_t reg_info = hgraph_registry_info(current_registry);
			log_debug("Number of data types: %d", reg_info.num_data_types);
			log_debug("Number of node types: %d", reg_info.num_node_types);
		}
	}

	if (op == REMODULE_OP_LOAD || op == REMODULE_OP_AFTER_RELOAD) {
		args->app = (sapp_desc){
			.init_userdata_cb = init,
			.cleanup_userdata_cb = cleanup,
			.event_cb = event,
			.frame_cb = frame,
			.window_title = "heditor",
			.width = 1280,
			.height = 720,
			.icon = op == REMODULE_OP_LOAD
				? load_app_icon(&frame_arena)
				: (sapp_icon_desc){ .sokol_default = true },
			.user_data = userdata,
			.logger = args->sokol_logger,
			.allocator = args->sokol_allocator,
		};
	}

	if (op == REMODULE_OP_AFTER_RELOAD && node_editor != NULL) {
		neSetCurrentEditor(node_editor);
	}
}
