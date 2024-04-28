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

// Registry
REMODULE_VAR(hgraph_registry_config_t, registry_config) = { 0 };
REMODULE_VAR(hgraph_registry_builder_t*, registry_builder) = NULL;
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

extern void
draw_editor(
	hed_arena_t* arena,
	node_type_menu_entry_t* node_type_menu,
	hgraph_t* graph
);

static void
build_registry(
	hed_allocator_t* alloc,
	size_t* size_ptr,
	hgraph_registry_t** registry_ptr
) {
	size_t required_size = hgraph_registry_init(NULL, registry_builder);

	hgraph_registry_t* registry = *registry_ptr;
	if (required_size > *size_ptr) {
		registry = hed_realloc(registry, required_size, alloc);
		*size_ptr = required_size;
		*registry_ptr = registry;
	}

	hgraph_registry_init(registry, registry_builder);
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
	size_t required_size = hgraph_init(NULL, &config);

	hgraph_t* graph = *graph_ptr;
	if (required_size > *size_ptr) {
		graph = hed_realloc(graph, required_size, alloc);
		*size_ptr = required_size;
		*graph_ptr = graph;
	}

	hgraph_init(graph, &config);
}

static int
new_document(hed_allocator_t* alloc) {
	if (num_documents >= editor_config.max_documents) { return -1; }

	int new_doc_index = num_documents++;

	document_t* document = &documents[new_doc_index];
	hed_free(document->path, alloc);
	document->path = NULL;
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

	hed_debug = hed_debug || is_debugger_attached();
	ImGuiIO* igIo = igGetIO();
	igIo->ConfigDebugIsDebuggerPresent = hed_debug;

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
}

static void
event(const sapp_event* ev) {
	simgui_handle_event(ev);
}

static void
frame(void* userdata) {
	entry_args_t* args = userdata;

	// Reload plugins
	bool should_reload_plugins = false;
	for (int i = 0; i < num_plugins; ++i) {
		bool plugin_updated = remodule_should_reload(plugin_monitors[i]);
		should_reload_plugins = should_reload_plugins || plugin_updated;
		if (plugin_updated) {
			log_info("Plugin updated: %s", remodule_path(plugins[i]));
		}
	}
	if (should_reload_plugins) {
		hgraph_registry_builder_init(registry_builder, &registry_config);
		for (int i = 0; i < num_plugins; ++i) {
			remodule_reload(plugins[i]);
		}

		// Migrate graphs
		build_registry(args->allocator, &next_registry_size, &next_registry);
		size_t required_migration_size = hgraph_migration_init(
			NULL, current_registry, next_registry
		);
		if (required_migration_size > migration_size) {
			migration = hed_realloc(
				migration, required_migration_size, args->allocator
			);
			migration_size = required_migration_size;
		}
		hgraph_migration_init(migration, current_registry, next_registry);

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
			NULL, current_registry, &frame_arena
		);
		if (required_menu_size > menu_size) {
			node_type_menu = hed_realloc(
				node_type_menu, required_menu_size, args->allocator
			);
			menu_size = required_menu_size;
		}
		node_type_menu_init(
			node_type_menu, current_registry, &frame_arena
		);
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

			if (igMenuItem_Bool("Save", "Ctrl+S", false, true)) {
				HED_CMD(HED_CMD_SAVE);
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

	static bool should_switch_tab = false;
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
					bool tab_open = true;
					bool tab_is_active = igBeginTabItem(
						tab_name.data,
						num_documents > 1 ? &tab_open : NULL,
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
							draw_editor(
								&frame_arena,
								node_type_menu,
								document->current_graph
							);
							nePopStyleVar(numStyleVars); numStyleVars = 0;
							nePopStyleColor(numStyleColors); numStyleColors = 0;
						}
						neEnd();
						neSetCurrentEditor(NULL);

						igEndTabItem();
					}

					if (tab_open) {
						++i;
					} else {
						--num_documents;
						SWAP(document_t, documents[i], documents[num_documents]);
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

	// Handle GUI commands
	if (show_imgui_demo) {
		igShowDemoWindow(&show_imgui_demo);
	}

	document_t* active_document = &documents[active_document_index];
	neSetCurrentEditor(active_document->node_editor);

	static bool should_navigate_to_content = false;
	if (should_navigate_to_content) {
		neNavigateToContent(-1.f);
		should_navigate_to_content = false;
	}

	hed_command_t cmd;
	while (hed_next_cmd(&cmd)) {
		switch (cmd.type) {
			case HED_CMD_EXIT:
				sapp_request_quit();
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
							FILE* file = fopen(path, "rb");
							if (file != NULL) {
								hgraph_config_t config = graph_config;
								config.registry = current_registry;
								hgraph_init(active_document->current_graph, &config);
								hgraph_io_status_t status = load_graph(active_document->current_graph, file);
								fclose(file);

								if (status == HGRAPH_IO_OK) {
									should_navigate_to_content = true;
									hed_free(active_document->path, args->allocator);
									active_document->path = hed_path_resolve(args->allocator, path);
								}
							} else {
								log_error("Could not open %s: %s", path, strerror(errno));
							}
						}

						NFD_FreePathU8(path);
					} else if (result == NFD_ERROR) {
						log_error("Could not open dialog: %s", NFD_GetError());
					}
				}
				break;
			case HED_CMD_SAVE:
				{
					char* path = NULL;

					nfdresult_t result = NFD_SaveDialogU8(
						&path,
						FILE_FILTERS, NUM_FILE_FILTERS,
						hed_path_as_str(project_root),
						NULL
					);

					if (result == NFD_OKAY) {
						FILE* file = fopen(path, "wb");
						if (file != NULL) {
							log_debug("Saving %s", path);
							save_graph(active_document->current_graph, file);
							fclose(file);
						} else {
							log_error("Could not open %s: %s", path, strerror(errno));
						}
						NFD_FreePathU8(path);
					} else if (result == NFD_ERROR) {
						log_error("Could not open dialog: %s", NFD_GetError());
					}
				}
				break;
			case HED_CMD_CREATE_NODE:
				{
					hgraph_index_t node_id = hgraph_create_node(
						active_document->current_graph,
						cmd.create_node_args.node_type
					);
					neSetNodePosition(node_id, cmd.create_node_args.pos);
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

				size_t registry_builder_size = hgraph_registry_builder_init(
					NULL, &registry_config
				);
				log_debug("Registry builder size: %zu", registry_builder_size);
				registry_builder = hed_malloc(registry_builder_size, args->allocator);
				hgraph_registry_builder_init(registry_builder, &registry_config);
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
				NULL, current_registry, &frame_arena
			);
			if (required_menu_size > menu_size) {
				node_type_menu = hed_realloc(
					node_type_menu, required_menu_size, args->allocator
				);
				menu_size = required_menu_size;
			}
			node_type_menu_init(
				node_type_menu, current_registry, &frame_arena
			);
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
}
