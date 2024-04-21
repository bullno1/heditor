#include "plugin_api_impl.h"
#include "utils.h"
#include <heditor/plugin.h>
#include <string.h>
#include "sfd/sfd.h"
#include "pico_log.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <errno.h>

static bool
begin_widget(hed_gui_t* ctx) {
	hed_plugin_api_impl_t* impl = HED_CONTAINER_OF(ctx, hed_plugin_api_impl_t, impl);
	int widget_index = ++impl->widget_index;
	if (impl->is_popup && widget_index != impl->popup_info->widget_index) {
		return false;
	} else {
		igPushID_Int(widget_index);
		return true;
	}
}

static void
end_widget(hed_gui_t* ctx) {
	(void)ctx;
	igPopID();
}

static bool
begin_popup(hed_gui_t* ctx, hed_ref_str_t label) {
	hed_plugin_api_impl_t* impl = HED_CONTAINER_OF(ctx, hed_plugin_api_impl_t, impl);
	int widget_index = ++impl->widget_index;

	if (impl->is_popup) {
		if (widget_index == impl->widget_index) {
			// Begin a nested inline context
			impl->saved_widget_index = impl->widget_index;
			impl->widget_index = 0;
			impl->is_popup = false;
			return true;
		} else {
			return false;
		}
	} else {
		igPushID_Int(widget_index);
		bool popup_requested = igButton(label.chars, (ImVec2){ 0.f, 0.f });
		igPopID();

		if (popup_requested && impl->popup_info->widget_index == 0) {
			impl->popup_info->widget_index = widget_index;
		}

		return false;
	}
}

static void
end_popup(hed_gui_t* ctx) {
	hed_plugin_api_impl_t* impl = HED_CONTAINER_OF(ctx, hed_plugin_api_impl_t, impl);
	impl->is_popup = true;
	impl->widget_index = impl->saved_widget_index;
}

static bool
render_label(
	hed_gui_t* ctx,
	hed_ref_str_t label,
	const hed_label_opts_t* options
) {
	if (!begin_widget(ctx)) { return false; }

	bool result = false;
	if (options->selectable) {
		result = igButton(label.chars, (ImVec2){ 0.f, 0.f });
	} else {
		igText("%.*s", (int)label.len, label.chars);
	}

	if (!options->line_break) {
		igSameLine(0.f, -1.f);
	}

	end_widget(ctx);
	return result;
}

static bool
render_scalar_input(
	hed_gui_t* ctx,
	hed_scalar_t* value,
	const hed_scalar_input_opts_t* options
) {
	if (!begin_widget(ctx)) { return false; }

	ImGuiDataType imgui_type;
	void* addr;
	const void* min;
	const void* max;
	float step;
	const void* step_ptr;
	switch (value->type) {
		case HED_SCALAR_INT:
			imgui_type = ImGuiDataType_S32;
			addr = &value->value.i32;
			min = &options->min.i32;
			max = &options->max.i32;
			step = (float)(options->step.i32);
			step_ptr = &options->step.i32;
			break;
		case HED_SCALAR_FLOAT:
			imgui_type = ImGuiDataType_Float;
			addr = &value->value.f32;
			min = &options->min.f32;
			max = &options->max.f32;
			step = options->step.f32;
			step_ptr = &options->step.f32;
			break;
		default:
			return false;
	}

	bool updated = false;

	igSetNextItemWidth(128.f);
	switch (options->type) {
		case HED_SCALAR_INPUT_SLIDER:
			updated = igDragScalar(
				"", imgui_type,
				addr,
				step,
				min, max,
				NULL,
				ImGuiSliderFlags_None
			);
			break;
		case HED_SCALAR_INPUT_TEXT_BOX:
			updated = igInputScalar(
				"", imgui_type,
				addr,
				step_ptr, NULL,
				NULL,
				ImGuiInputTextFlags_None
			);
			break;
	}

	end_widget(ctx);
	return updated;
}

static bool
render_text_input_line(
	struct hed_gui_s* ctx,
	hed_editable_str_t* value,
	const hed_text_input_opts_t* options
) {
	(void)options;
	if (!begin_widget(ctx)) { return false; }

	igSetNextItemWidth(256.f);
	bool updated = igInputText(
		"",
		value->chars, value->capacity,
		ImGuiInputTextFlags_None,
		NULL, NULL
	);

	end_widget(ctx);
	return updated;
}

static bool
render_text_input_area(
	struct hed_gui_s* ctx,
	hed_editable_str_t* value,
	const hed_text_input_opts_t* options
) {
	(void)options;
	hed_ref_str_t label = {
		.chars = value->chars,
		.len = value->len
	};
	if (begin_popup(ctx, label)) {
		bool updated = igInputTextMultiline(
			"##Text",
			value->chars, value->capacity,
			(ImVec2){ 0.f, 0.f },
			ImGuiInputTextFlags_None,
			NULL, NULL
		);
		end_popup(ctx);
		return updated;
	} else {
		return false;
	}
}

static bool
render_text_input_file_picker(
	struct hed_gui_s* ctx,
	hed_editable_str_t* value,
	const hed_text_input_opts_t* options
) {
	(void)options;

	hed_ref_str_t label = {
		.chars = value->chars,
		.len = value->len
	};

	bool updated = false;
	if (render_label(ctx, label, &(hed_label_opts_t){ .selectable = true })) {
		// TODO: set project root
		const char* path = sfd_open_dialog(&(sfd_Options){
			.title = "Choose a file",
		});

		if (path == NULL) {
			const char* error = sfd_get_error();
			if (error) {
				log_error("%s (%d)", error, strerror(errno));
			}
		} else {
			size_t len = strlen(path);
			if (len <= value->capacity) {
				memcpy(value->chars, path, len);
				value->chars[len] = '\0';
				updated = true;
			}
		}
	}

	return updated;
}

static bool
render_text_input(
	struct hed_gui_s* ctx,
	hed_editable_str_t* value,
	const hed_text_input_opts_t* options
) {
	bool updated;
	switch (options->type) {
		case HED_TEXT_INPUT_LINE:
			updated = render_text_input_line(ctx, value, options);
			break;
		case HED_TEXT_INPUT_AREA:
			updated = render_text_input_area(ctx, value, options);
			break;
		case HED_TEXT_INPUT_FILE_PICKER:
			updated = render_text_input_file_picker(ctx, value, options);
			break;
	}

	if (updated) {
		value->len = strlen(value->chars);
	}

	return updated;
}

static bool
render_enum_input(
	hed_gui_t* ctx,
	int32_t* value,
	const hed_enum_input_opts_t* opts
) {
	if (begin_popup(ctx, opts->labels[*value])) {
		bool updated = false;

		for (int i = 0; i < opts->count; ++i) {
			if (igSelectable_Bool(
				opts->labels[i].chars,
				i == *value,
				ImGuiSliderFlags_None,
				(ImVec2){ 0.f, 0.f }
			)) {
				*value = i;
				updated = true;
			}
		}

		end_popup(ctx);
		return updated;
	} else {
		return false;
	}
}

hed_gui_t*
hed_gui_init(
	hed_plugin_api_impl_t* impl,
	hed_gui_popup_info_t* popup_info,
	bool is_popup,
	struct ImGuiContext* imgui_ctx
) {
	*impl = (hed_plugin_api_impl_t){
		.impl = {
			.imgui_ctx = imgui_ctx,
			.render_scalar_input = render_scalar_input,
			.render_text_input = render_text_input,
			.render_enum_input = render_enum_input,
			.render_label = render_label,
			.begin_popup = begin_popup,
			.end_popup = end_popup,
		},
		.popup_info = popup_info,
		.is_popup = is_popup
	};

	return &impl->impl;
}

bool
hed_gui_popup_requested(hed_gui_popup_info_t* popup_info) {
	return popup_info->widget_index > 0;
}
