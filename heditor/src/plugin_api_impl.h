#ifndef HEDITOR_PLUGIN_API_IMPL_H
#define HEDITOR_PLUGIN_API_IMPL_H

#include <heditor/plugin.h>

struct ImGuiContext;

typedef struct hed_gui_popup_info_s {
	int widget_index;
} hed_gui_popup_info_t;

typedef struct hed_plugin_api_impl_s {
	hed_gui_t impl;
	int widget_index;
	int saved_widget_index;
	hed_gui_popup_info_t* popup_info;
	bool is_popup;
} hed_plugin_api_impl_t;

hed_gui_t*
hed_gui_init(
	hed_plugin_api_impl_t* impl,
	hed_gui_popup_info_t* popup_info,
	bool is_popup,
	struct ImGuiContext* imgui_ctx
);

bool
hed_gui_popup_requested(hed_gui_popup_info_t* popup_info);

#endif
