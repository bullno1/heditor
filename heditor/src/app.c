#include <remodule_monitor.h>
#define REMODULE_PLUGIN_IMPLEMENTATION
#include <remodule.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <util/sokol_imgui.h>
#include <nfd.h>
#include "pico_log.h"
#include "command.h"
#include "detect_debugger.h"
#include "entry.h"
#include "app.h"
#include "allocator/arena.h"
#include "cnode-editor.h"
#include "node_type_menu.h"
#include "resources.h"
#include "io.h"
#include "pipeline_runner.h"
#include <xincbin.h>
#include <string.h>
#include <errno.h>
#include <float.h>

#define PLUGIN_PREFIX "lib"
#ifndef NDEBUG
#	define PLUGIN_SUFFIX "_d"
#else
#	define PLUGIN_SUFFIX ""
#endif

#define SWAP(TYPE, A, B) \
	do { \
		TYPE tmp = A; \
		A = B; \
		B = tmp; \
	} while (0)

#ifdef NDEBUG
REMODULE_VAR(bool, hed_debug) = false;
#else
REMODULE_VAR(bool, hed_debug) = true;
#endif

typedef struct {
	size_t current_graph_size;
	hgraph_t* current_graph;
	size_t next_graph_size;
	hgraph_t* next_graph;

	int navigate_to_content;
	bool is_dirty;

	hed_path_t* path;
	neEditorContext* node_editor;
} document_t;

// Globals
REMODULE_VAR(bool, show_imgui_demo) = false;
REMODULE_VAR(hed_arena_t, frame_arena) = { 0 };

// Plugins
REMODULE_VAR(int, num_plugins) = 0;
REMODULE_VAR(remodule_t**, plugins) = NULL;
REMODULE_VAR(remodule_monitor_t**, plugin_monitors) = NULL;
REMODULE_VAR(bool, should_reload_plugins) = false;

// Registry
REMODULE_VAR(hgraph_registry_config_t, registry_config) = { 0 };
REMODULE_VAR(hgraph_registry_builder_t*, registry_builder) = NULL;
REMODULE_VAR(size_t, registry_builder_size) = 0;
REMODULE_VAR(size_t, current_registry_size) = 0;
REMODULE_VAR(hgraph_registry_t*, current_registry) = NULL;
REMODULE_VAR(size_t, next_registry_size) = 0;
REMODULE_VAR(hgraph_registry_t*, next_registry) = NULL;
REMODULE_VAR(size_t, migration_size) = 0;
REMODULE_VAR(hgraph_migration_t*, migration) = NULL;

// Node menu
REMODULE_VAR(size_t, menu_size) = 0;
REMODULE_VAR(node_type_menu_entry_t*, node_type_menu) = 0;

// Shared doc config
REMODULE_VAR(hgraph_config_t, graph_config) = { 0 };
REMODULE_VAR(hed_path_t*, project_root) = NULL;
REMODULE_VAR(editor_config_t, editor_config) = DEFAULT_EDITOR_CONFIG;

// Document properties
REMODULE_VAR(int, num_documents) = 0;
REMODULE_VAR(int, active_document_index) = 0;
REMODULE_VAR(document_t*, documents) = NULL;

// Pipeline
REMODULE_VAR(hgraph_pipeline_config_t, pipeline_config) = { 0 };
REMODULE_VAR(size_t, current_pipeline_size) = 0;
REMODULE_VAR(hgraph_pipeline_t*, current_pipeline) = NULL;
REMODULE_VAR(size_t, next_pipeline_size) = 0;
REMODULE_VAR(hgraph_pipeline_t*, next_pipeline) = NULL;
static pipeline_runner_t pipeline_runner;
static hgraph_t* pipeline_bound_graph = NULL;

static const nfdu8filteritem_t FILE_FILTERS[] = {
	{
		.name = "Graph file",
		.spec = "hed"
	}
};
static const nfdfiltersize_t NUM_FILE_FILTERS =
	sizeof(FILE_FILTERS) / sizeof(FILE_FILTERS[0]);

extern sapp_icon_desc
load_app_icon(hed_arena_t* arena);

extern app_config_t*
load_app_config(hed_arena_t* arena);

extern bool
draw_editor(
	hed_arena_t* arena,
	bool editable,
	node_type_menu_entry_t* node_type_menu,
	hgraph_t* graph
);

static void
build_registry(
	hed_allocator_t* alloc,
	size_t* size_ptr,
	hgraph_registry_t** registry_ptr
) {
	hgraph_registry_t* registry = *registry_ptr;
	size_t size = *size_ptr;
	size_t required_size = hgraph_registry_init(registry, size, registry_builder);

	if (required_size > size) {
		size = required_size;
		registry = hed_realloc(registry, required_size, alloc);
		hgraph_registry_init(registry, size, registry_builder);
		*registry_ptr = registry;
		*size_ptr = required_size;
	}
}

static void
build_graph(
	hed_allocator_t* alloc,
	hgraph_registry_t* registry,
	size_t* size_ptr,
	hgraph_t** graph_ptr
) {
	hgraph_config_t config = graph_config;
	config.registry = registry;

	hgraph_t* graph = *graph_ptr;
	size_t size = *size_ptr;
	size_t required_size = hgraph_init(graph, size, &config);

	if (required_size > size) {
		size = required_size;
		graph = hed_realloc(graph, required_size, alloc);
		hgraph_init(graph, size, &config);
		*size_ptr = required_size;
		*graph_ptr = graph;
	}
}

static int
new_document(hed_allocator_t* alloc) {
	if (num_documents >= editor_config.max_documents) { return -1; }

	int new_doc_index = num_documents++;

	document_t* document = &documents[new_doc_index];
	hed_free(document->path, alloc);
	document->path = NULL;
	document->navigate_to_content = 0;
	document->is_dirty = false;
	if (document->node_editor == NULL) {
		neConfig config = neConfigDefault();
		config.EnableSmoothZoom = true;
		config.NavigateButtonIndex = 2;
		document->node_editor = neCreateEditor(&config);
	}
	build_graph(alloc, current_registry, &document->current_graph_size, &document->current_graph);

	return new_doc_index;
}

static void
close_document(int index) {
	if (0 <= index && index < num_documents) {
		--num_documents;
		SWAP(document_t, documents[index], documents[num_documents]);
	}
}

static bool
save_document_at(document_t* document, const char* path) {
	FILE* file = fopen(path, "wb");
	if (file != NULL) {
		log_debug("Saving %s", path);
		neEditorContext* editor = neGetCurrentEditor();
		neSetCurrentEditor(document->node_editor);
		hgraph_io_status_t status = save_graph(document->current_graph, file);
		neSetCurrentEditor(editor);
		fclose(file);
		if (status == HGRAPH_IO_OK) {
			document->is_dirty = false;
		}

		return status == HGRAPH_IO_OK;
	} else {
		log_error("Could not open %s: %s", path, strerror(errno));
		return false;
	}
}

static bool
hed_cmd_save_as(document_t* document, hed_allocator_t* alloc) {
	bool saved = false;
	char* path = NULL;

	nfdresult_t result = NFD_SaveDialogU8(
		&path,
		FILE_FILTERS, NUM_FILE_FILTERS,
		hed_path_as_str(project_root),
		NULL
	);

	if (result == NFD_OKAY) {
		if (save_document_at(document, path)) {
			hed_free(document->path, alloc);
			document->path = hed_path_resolve(alloc, path);
			saved = true;
		}
		NFD_FreePathU8(path);
	} else if (result == NFD_ERROR) {
		log_error("Could not open dialog: %s", NFD_GetError());
	}

	return saved;
}

static bool
hed_cmd_save(document_t* document, hed_allocator_t* alloc) {
	if (document->path == NULL) {
		return hed_cmd_save_as(document, alloc);
	} else {
		return save_document_at(document, hed_path_as_str(document->path));
	}
}

static bool
should_confirm_exit(void) {
	for (int i = 0; i < num_documents; ++i) {
		if (documents[i].is_dirty) {
			return true;
		}
	}

	return false;
}

static void
init(void* userdata) {
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

	NFD_Init();

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
		.no_default_font = true,
	});

	ImGuiIO* igIo = igGetIO();
	igIo->ConfigDebugIsDebuggerPresent = is_debugger_attached();

	xincbin_data_t font_data = XINCBIN_GET(font);
	ImFontAtlas_AddFontFromMemoryTTF(
		igIo->Fonts,
		(void*)font_data.data,
		font_data.size,
		16.f,
		&(ImFontConfig){
			.FontDataOwnedByAtlas = false,
			.OversampleH = 2,
			.OversampleV = 2,
			.RasterizerMultiply = 1.5f,
			.RasterizerDensity = 1.0f,
			.GlyphMaxAdvanceX = FLT_MAX,
			.EllipsisChar = (ImWchar)-1,
		},
		NULL
	);

	simgui_font_tex_desc_t font_texture_desc = { 0 };
	font_texture_desc.min_filter = SG_FILTER_LINEAR;
	font_texture_desc.mag_filter = SG_FILTER_LINEAR;
	simgui_create_fonts_texture(&font_texture_desc);

	pipeline_runner_init(&pipeline_runner);
}

static void
event(const sapp_event* ev) {
	if (ev->type == SAPP_EVENTTYPE_QUIT_REQUESTED) {
		if (should_confirm_exit()) {
			sapp_cancel_quit();
			HED_CMD(HED_CMD_EXIT);
		}
	}

	simgui_handle_event(ev);
}

static void
frame(void* userdata) {
	entry_args_t* args = userdata;

	// Reload plugins
	for (int i = 0; i < num_plugins; ++i) {
		bool plugin_updated = remodule_should_reload(plugin_monitors[i]);
		should_reload_plugins = should_reload_plugins || plugin_updated;
		if (plugin_updated) {
			log_info("Plugin updated: %s", remodule_path(plugins[i]));
		}
	}
	if (should_reload_plugins) {
		// TODO: Add a synchronous stop
		pipeline_runner_terminate(&pipeline_runner);

		hgraph_registry_builder_init(registry_builder, registry_builder_size, &registry_config);
		for (int i = 0; i < num_plugins; ++i) {
			remodule_reload(plugins[i]);
		}

		// Migrate graphs
		build_registry(args->allocator, &next_registry_size, &next_registry);
		size_t required_migration_size = hgraph_migration_init(
			migration, migration_size, current_registry, next_registry
		);
		if (required_migration_size > migration_size) {
			migration = hed_realloc(
				migration, required_migration_size, args->allocator
			);
			migration_size = required_migration_size;
			hgraph_migration_init(
				migration, migration_size, current_registry, next_registry
			);
		}

		for (int i = 0; i < num_documents; ++i) {
			document_t* document = &documents[i];

			build_graph(
				args->allocator,
				next_registry,
				&document->next_graph_size, &document->next_graph
			);
			hgraph_migration_execute(
				migration, document->current_graph, document->next_graph
			);

			SWAP(size_t, document->current_graph_size, document->next_graph_size);
			SWAP(hgraph_t*, document->current_graph, document->next_graph);
		}

		// Swap registry
		SWAP(size_t, current_registry_size, next_registry_size);
		SWAP(hgraph_registry_t*, current_registry, next_registry);

		// Rebuild node menu
		size_t required_menu_size = node_type_menu_init(
			node_type_menu, menu_size, current_registry, &frame_arena
		);
		if (required_menu_size > menu_size) {
			node_type_menu = hed_realloc(
				node_type_menu, required_menu_size, args->allocator
			);
			menu_size = required_menu_size;
			node_type_menu_init(
				node_type_menu, menu_size, current_registry, &frame_arena
			);
		}

		pipeline_runner_init(&pipeline_runner);
		should_reload_plugins = false;
	}

	// GUI
	simgui_new_frame(&(simgui_frame_desc_t){
		.width = sapp_width(),
		.height = sapp_height(),
		.delta_time = sapp_frame_duration(),
		.dpi_scale = sapp_dpi_scale(),
	});

	bool can_create_new_document = num_documents < editor_config.max_documents;

	// Main menu
	if (igBeginMainMenuBar()) {
		if (igBeginMenu("File", true)) {
			if (igMenuItem_Bool("New", "Ctr+N", false, can_create_new_document)) {
				HED_CMD(HED_CMD_NEW);
			}

			if (igMenuItem_Bool("Open", "Ctrl+O", false, true)) {
				HED_CMD(HED_CMD_OPEN);
			}

			igSeparator();

			if (igMenuItem_Bool("Save", "Ctrl+S", false, true)) {
				HED_CMD(HED_CMD_SAVE);
			}

			if (igMenuItem_Bool("Save as", "Ctrl+Shift+S", false, true)) {
				HED_CMD(HED_CMD_SAVE_AS);
			}

			igSeparator();

			if (igMenuItem_Bool("Exit", NULL, false, true)) {
				HED_CMD(HED_CMD_EXIT);
			}

			igEndMenu();
		}

		pipeline_runner_state_t runner_state = pipeline_runner_current_state(&pipeline_runner);
		if (igBeginMenu("Run", true)) {
			if (igMenuItem_Bool("Play", "F5", false, runner_state == PIPELINE_STOPPED)) {
				HED_CMD(HED_CMD_EXECUTE);
			}

			if (igMenuItem_Bool("Pause", "F6", false, runner_state == PIPELINE_RUNNING)) {
			}

			if (igMenuItem_Bool("Resume", "F7", false, runner_state == PIPELINE_PAUSED)) {
			}

			if (igMenuItem_Bool("Stop", "Ctrl+C", false, runner_state != PIPELINE_STOPPED)) {
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

	static bool should_switch_tab = false;
	static int dirty_doc_index = -1;
	igBegin("Main", NULL, main_window_flags);
	{
		ImGuiTabBarFlags tab_bar_flags =
			ImGuiTabBarFlags_Reorderable;
		if (igBeginTabBar("Documents", tab_bar_flags)) {
			for (int i = 0; i < num_documents;) {
				HED_WITH_ARENA(&frame_arena) {
					document_t* document = &documents[i];
					hed_allocator_t* alloc = hed_arena_as_allocator(&frame_arena);

					hed_str_t tab_name = hed_str_format(
						alloc,
						"%s###%p",
						document->path != NULL
							? hed_path_basename(alloc, document->path)
							: "Untitled",
						(void*)document->node_editor
					);

					ImGuiTabItemFlags tab_item_flags = ImGuiTabItemFlags_None;
					if (should_switch_tab && i == active_document_index) {
						tab_item_flags |= ImGuiTabItemFlags_SetSelected;
					}
					if (document->is_dirty) {
						tab_item_flags |= ImGuiTabItemFlags_UnsavedDocument;
					}

					bool tab_open = true;
					bool tab_is_active = igBeginTabItem(
						tab_name.data,
						&tab_open,
						tab_item_flags
					);
					if (
						document->path != NULL
						&& igIsItemHovered(
							ImGuiHoveredFlags_ForTooltip
							| ImGuiHoveredFlags_DelayShort
						)
					) {
						igSetTooltip(hed_path_as_str(document->path));
					}
					if (tab_is_active) {
						if (!should_switch_tab) { active_document_index = i; }

						neSetCurrentEditor(document->node_editor);
						neBegin("Editor", (ImVec2){ 0 });
						{
							int numStyleVars = 0;
							int numStyleColors = 0;
							nePushStyleColor(neStyleColor_PinRect, (ImVec4){ 1.f, 1.f, 1.f, 0.7f }); ++numStyleColors;
							nePushStyleVarFloat(neStyleVar_NodeRounding, 4.f); ++numStyleVars;
							nePushStyleVarVec4(neStyleVar_NodePadding, (ImVec4){ 8.f, 4.f, 8.f, 4.f }); ++numStyleVars;
							bool editable = document->current_graph != pipeline_bound_graph
								|| pipeline_runner_current_state(&pipeline_runner) == PIPELINE_STOPPED;
							bool updated = draw_editor(
								&frame_arena,
								editable,
								node_type_menu,
								document->current_graph
							);
							nePopStyleVar(numStyleVars); numStyleVars = 0;
							nePopStyleColor(numStyleColors); numStyleColors = 0;
							document->is_dirty = document->is_dirty || updated;

							if (
									document->navigate_to_content > 0
									&& --document->navigate_to_content == 0
							) {
								neNavigateToContent(-1.f);
							}
						}
						neEnd();
						neSetCurrentEditor(NULL);

						igEndTabItem();
					}

					if (tab_open) {
						++i;
					} else if (!document->is_dirty) {
						close_document(i);
					} else {
						dirty_doc_index = i;
					}
				}
			}

			if (
				can_create_new_document
				&& igTabItemButton("+", ImGuiTabItemFlags_Trailing)
			) {
				HED_CMD(HED_CMD_NEW);
			}

			igEndTabBar();
		}
		should_switch_tab = false;
	}
	igEnd();

	// Hot keys
	if (igIsKeyChordPressed_Nil(ImGuiKey_N | ImGuiMod_Ctrl) && can_create_new_document) {
		HED_CMD(HED_CMD_NEW);
	}

	if (igIsKeyChordPressed_Nil(ImGuiKey_O | ImGuiMod_Ctrl)) {
		HED_CMD(HED_CMD_OPEN);
	}

	if (igIsKeyChordPressed_Nil(ImGuiKey_S | ImGuiMod_Ctrl)) {
		HED_CMD(HED_CMD_SAVE);
	}

	if (igIsKeyChordPressed_Nil(ImGuiKey_S | ImGuiMod_Ctrl | ImGuiMod_Shift)) {
		HED_CMD(HED_CMD_SAVE_AS);
	}

	// Handle GUI commands
	if (show_imgui_demo) {
		igShowDemoWindow(&show_imgui_demo);
	}

	if (dirty_doc_index >= 0) {
		if (!igIsPopupOpen_Str("Save?", ImGuiPopupFlags_None)) {
			igOpenPopup_Str("Save?", ImGuiPopupFlags_None);
			active_document_index = dirty_doc_index;
			should_switch_tab = true;
		}

		if (igBeginPopupModal("Save?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			document_t* document = &documents[dirty_doc_index];

			HED_WITH_ARENA(&frame_arena) {
				if (document->path == NULL) {
					igText(
						"This document has been modified.\n"
						"\n"
						"Do you want to save it?"
					);
				} else {
					igText(
						"The document '%s' has been modified.\n"
						"\n"
						"Do you want to save it?",
						hed_path_basename(hed_arena_as_allocator(&frame_arena), document->path)
					);
				}
			}

			bool can_close_popup = false;
			if (igButton("Yes", (ImVec2){ 0 })) {
				if (hed_cmd_save(document, args->allocator)) {
					close_document(dirty_doc_index);
					can_close_popup = true;
				}
			}
			igSameLine(0.f, -1.f);

			if (igButton("No", (ImVec2){ 0 })) {
				close_document(dirty_doc_index);
				can_close_popup = true;
			}
			igSameLine(0.f, -1.f);

			if (igButton("Cancel", (ImVec2){ 0 })) {
				can_close_popup = true;
			}
			igSameLine(0.f, -1.f);

			if (can_close_popup) {
				dirty_doc_index = -1;
				igCloseCurrentPopup();
			}

			igEndPopup();
		}
	}

	if (igBeginPopupModal("Exit?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		igText(
			"You have unsaved changes.\n"
			"\n"
			"Are you sure you want to exit?"
		);

		if (igButton("Yes", (ImVec2){ 0 })) {
			sapp_quit();
		}
		igSameLine(0.f, -1.f);

		if (igButton("No", (ImVec2){ 0 })) {
			igCloseCurrentPopup();
		}

		igEndPopup();
	}

	document_t* active_document = &documents[active_document_index];
	neSetCurrentEditor(active_document->node_editor);

	hed_command_t cmd;
	while (hed_next_cmd(&cmd)) {
		switch (cmd.type) {
			case HED_CMD_EXIT:
				{
					if (should_confirm_exit()) {
						igOpenPopup_Str("Exit?", ImGuiPopupFlags_None);
					} else {
						sapp_quit();
					}
				}
				break;
			case HED_CMD_NEW:
				{
					int new_doc_index = new_document(args->allocator);
					if (new_doc_index < 0) { continue; }
					active_document_index = new_doc_index;
					should_switch_tab = true;
				}
				break;
			case HED_CMD_OPEN:
				{
					char* path;

					nfdresult_t result = NFD_OpenDialogU8(
						&path,
						FILE_FILTERS, NUM_FILE_FILTERS,
						hed_path_as_str(project_root)
					);

					if (result == NFD_OKAY) {
						int existing_tab = -1;
						for (int i = 0; i < num_documents; ++i) {
							document_t* document = &documents[i];
							if (
								document->path != NULL
								&& strcmp(path, hed_path_as_str(document->path)) == 0
							) {
								existing_tab = i;
								break;
							}
						}

						if (existing_tab >= 0) {
							active_document_index = existing_tab;
							should_switch_tab = true;
						} else {
							int new_doc_index = new_document(args->allocator);
							if (new_doc_index >= 0) {
								active_document_index = new_doc_index;
								should_switch_tab = true;
								document_t* new_doc = &documents[new_doc_index];

								FILE* file = fopen(path, "rb");
								if (file != NULL) {
									hgraph_io_status_t status;
									neEditorContext* editor = neGetCurrentEditor();
									neSetCurrentEditor(new_doc->node_editor);
									{
										hgraph_config_t config = graph_config;
										config.registry = current_registry;
										hgraph_init(new_doc->current_graph, new_doc->current_graph_size, &config);
										status = load_graph(new_doc->current_graph, file);
										fclose(file);
									}
									neSetCurrentEditor(editor);

									if (status == HGRAPH_IO_OK) {
										new_doc->navigate_to_content = 2;  // Delay nav for 2 frames
										hed_free(new_doc->path, args->allocator);
										new_doc->path = hed_path_resolve(args->allocator, path);
									}
								} else {
									log_error("Could not open %s: %s", path, strerror(errno));
								}
							}
						}

						NFD_FreePathU8(path);
					} else if (result == NFD_ERROR) {
						log_error("Could not open dialog: %s", NFD_GetError());
					}
				}
				break;
			case HED_CMD_SAVE:
				hed_cmd_save(active_document, args->allocator);
				break;
			case HED_CMD_SAVE_AS:
				hed_cmd_save_as(active_document, args->allocator);
				break;
			case HED_CMD_EXECUTE:
				{
					if (
						pipeline_runner_current_state(&pipeline_runner) != PIPELINE_STOPPED
					) {
						continue;
					}

					// Rebind pipeline
					if (active_document->current_graph != pipeline_bound_graph) {
						if (current_pipeline != NULL) {
							hgraph_pipeline_cleanup(current_pipeline);
						}

						hgraph_pipeline_config_t config = pipeline_config;
						config.graph = active_document->current_graph;
						size_t required_size = hgraph_pipeline_init(
							current_pipeline, current_pipeline_size, &config
						);
						if (required_size > current_pipeline_size) {
							current_pipeline = hed_realloc(
								current_pipeline, required_size, args->allocator
							);
							current_pipeline_size = required_size;
							hgraph_pipeline_init(
								current_pipeline, current_pipeline_size, &config
							);
						}

						pipeline_bound_graph = active_document->current_graph;
					}

					hgraph_pipeline_config_t config = pipeline_config;
					config.graph = active_document->current_graph;
					config.previous_pipeline = current_pipeline;
					size_t required_size = hgraph_pipeline_init(
						next_pipeline, next_pipeline_size, &config
					);
					if (required_size > next_pipeline_size) {
						next_pipeline = hed_realloc(
							next_pipeline, required_size, args->allocator
						);
						next_pipeline_size = required_size;
						hgraph_pipeline_init(
							next_pipeline, next_pipeline_size, &config
						);
					}
					hgraph_pipeline_cleanup(current_pipeline);
					SWAP(hgraph_pipeline_t*, current_pipeline, next_pipeline);
					SWAP(size_t, current_pipeline_size, next_pipeline_size);

					pipeline_runner_execute(&pipeline_runner, current_pipeline);
				}
				break;
			case HED_CMD_CREATE_NODE:
				{
					hgraph_index_t node_id = hgraph_create_node(
						active_document->current_graph,
						cmd.create_node_args.node_type
					);
					neSetNodePosition(node_id, cmd.create_node_args.pos);
					active_document->is_dirty = true;
				}
				break;
			case HED_CMD_CREATE_EDGE:
				{
					hgraph_index_t edge_id = hgraph_connect(
						active_document->current_graph,
						cmd.create_edge_args.from_pin,
						cmd.create_edge_args.to_pin
					);
					if (HGRAPH_IS_VALID_INDEX(edge_id)) {
						neLink(
							edge_id,
							cmd.create_edge_args.from_pin,
							cmd.create_edge_args.to_pin
						);
						active_document->is_dirty = true;
					}
				}
				break;
		}
	}

	neSetCurrentEditor(NULL);

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

	pipeline_runner_terminate(&pipeline_runner);
	if (pipeline_bound_graph != NULL) {
		hgraph_pipeline_cleanup(current_pipeline);
	}

	hed_free(current_pipeline, args->allocator);
	hed_free(next_pipeline, args->allocator);

	for (int i = 0; i < editor_config.max_documents; ++i) {
		hed_free(documents[i].current_graph, args->allocator);
		hed_free(documents[i].next_graph, args->allocator);
		hed_free(documents[i].path, args->allocator);
		if (documents[i].node_editor != NULL) {
			neDestroyEditor(documents[i].node_editor);
		}
	}
	hed_free(documents, args->allocator);

	hed_free(project_root, args->allocator);
	hed_free(node_type_menu, args->allocator);

	hed_free(migration, args->allocator);
	hed_free(current_registry, args->allocator);
	hed_free(next_registry, args->allocator);

	for (int i = num_plugins - 1; i >= 0; --i) {
		remodule_unmonitor(plugin_monitors[i]);
		remodule_unload(plugins[i]);
	}
	hed_free(plugin_monitors, args->allocator);
	hed_free(plugins, args->allocator);

	hed_free(registry_builder, args->allocator);

	simgui_shutdown();
	NFD_Quit();
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

				registry_config = config->registry_config;
				graph_config = config->graph_config;
				project_root = hed_path_dup(args->allocator, config->project_root);
				editor_config = config->editor_config;
				pipeline_config = config->pipeline_config;

				registry_builder_size = hgraph_registry_builder_init(
					NULL, 0, &registry_config
				);
				log_debug("Registry builder size: %zu", registry_builder_size);
				registry_builder = hed_malloc(registry_builder_size, args->allocator);
				hgraph_registry_builder_init(registry_builder, registry_builder_size, &registry_config);
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
			build_registry(args->allocator, &current_registry_size, &current_registry);
			hgraph_registry_info_t reg_info = hgraph_registry_info(current_registry);
			log_debug("Number of data types: %d", reg_info.num_data_types);
			log_debug("Number of node types: %d", reg_info.num_node_types);

			size_t required_menu_size = node_type_menu_init(
				node_type_menu, menu_size, current_registry, &frame_arena
			);
			if (required_menu_size > menu_size) {
				node_type_menu = hed_realloc(
					node_type_menu, required_menu_size, args->allocator
				);
				menu_size = required_menu_size;
				node_type_menu_init(
					node_type_menu, menu_size, current_registry, &frame_arena
				);
			}
		}

		documents = hed_malloc(
			sizeof(document_t) * editor_config.max_documents,
			args->allocator
		);
		memset(documents, 0, sizeof(document_t) * editor_config.max_documents);
		active_document_index = new_document(args->allocator);
	}

	if (op == REMODULE_OP_LOAD || op == REMODULE_OP_AFTER_RELOAD) {
		args->app = (sapp_desc){
			.init_userdata_cb = init,
			.cleanup_userdata_cb = cleanup,
			.event_cb = event,
			.frame_userdata_cb = frame,
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

	if (op == REMODULE_OP_BEFORE_RELOAD) {
		pipeline_runner_terminate(&pipeline_runner);
	}
	if (op == REMODULE_OP_AFTER_RELOAD) {
		pipeline_runner_init(&pipeline_runner);
	}
}
