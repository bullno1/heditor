#ifndef HEDITOR_PLUGIN_H
#define HEDITOR_PLUGIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define HED_REF_STR(STR) \
	(hed_ref_str_t){ .len = strlen(STR) - 1, .chars = STR }

typedef enum hed_scalar_type_e {
	HED_SCALAR_FLOAT,
	HED_SCALAR_INT,
} hed_scalar_type_t;

typedef union hed_scalar_value_u {
	float f32;
	int32_t i32;
} hed_scalar_value_t;

typedef struct hed_scalar_s {
	hed_scalar_type_t type;
	hed_scalar_value_t value;
} hed_scalar_t;

typedef enum hed_scalar_input_type_e {
	HED_SCALAR_INPUT_SLIDER,
	HED_SCALAR_INPUT_TEXT_BOX,
} hed_scalar_input_type_t;

typedef struct hed_scalar_input_opts_s {
	hed_scalar_input_type_t type;
	hed_scalar_value_t min;
	hed_scalar_value_t max;
	hed_scalar_value_t step;
} hed_scalar_input_opts_t;

typedef enum hed_text_input_type_e {
	HED_TEXT_INPUT_LINE,
	HED_TEXT_INPUT_AREA,
	HED_TEXT_INPUT_FILE_PICKER,
} hed_text_input_type_t;

typedef struct hed_editable_str_s {
	size_t capacity;
	size_t len;
	char* chars;
} hed_editable_str_t;

typedef struct hed_ref_str_s {
	size_t len;
	const char* chars;
} hed_ref_str_t;

typedef struct hed_text_input_opts_s {
	hed_text_input_type_t type;
} hed_text_input_opts_t;

typedef struct hed_enum_input_opts_s {
	int count;
	hed_ref_str_t* labels;
} hed_enum_input_opts_t;

typedef struct hed_label_opts_s {
	bool line_break;
	bool selectable;
} hed_label_opts_t;

struct hed_gui_s;

typedef struct hed_gui_s {
	bool (*render_scalar_input)(
		struct hed_gui_s* ctx,
		hed_scalar_t* value,
		const hed_scalar_input_opts_t* options
	);
	bool (*render_text_input)(
		struct hed_gui_s* ctx,
		hed_editable_str_t* value,
		const hed_text_input_opts_t* options
	);
	bool (*render_enum_input)(
		struct hed_gui_s* ctx,
		int* value,
		const hed_enum_input_opts_t* opts
	);
	bool (*render_label)(
		struct hed_gui_s* ctx,
		hed_ref_str_t label,
		const hed_label_opts_t* options
	);
	bool (*begin_popup)(struct hed_gui_s* ctx, hed_ref_str_t label);
	void (*end_popup)(struct hed_gui_s* ctx);

	void* imgui_ctx;
} hed_gui_t;

static inline bool
hed_gui_render_scalar_input(
	struct hed_gui_s* ctx,
	hed_scalar_t* value,
	const hed_scalar_input_opts_t* options
) {
	return ctx->render_scalar_input(ctx, value, options);
}

static inline bool
hed_gui_render_f32_input(
	struct hed_gui_s* ctx,
	float* value,
	const hed_scalar_input_opts_t* options
) {
	hed_scalar_t tmp = {
		.type = HED_SCALAR_FLOAT,
		.value.f32 = *value,
	};
	bool result = hed_gui_render_scalar_input(ctx, &tmp, options);
	if (result) { *value = tmp.value.f32; }
	return result;
}

static inline bool
hed_gui_render_i32_input(
	struct hed_gui_s* ctx,
	int32_t* value,
	const hed_scalar_input_opts_t* options
) {
	hed_scalar_t tmp = {
		.type = HED_SCALAR_INT,
		.value.i32 = *value,
	};
	bool result = hed_gui_render_scalar_input(ctx, &tmp, options);
	if (result) { *value = tmp.value.i32; }
	return result;
}

static inline bool
hed_gui_render_string_input(
	struct hed_gui_s* ctx,
	hed_editable_str_t* value,
	const hed_text_input_opts_t* options
) {
	return ctx->render_text_input(ctx, value, options);
}

static inline bool
hed_gui_render_enum_input(
	struct hed_gui_s* ctx,
	int* value,
	const hed_enum_input_opts_t* options
) {
	return ctx->render_enum_input(ctx, value, options);
}

static inline bool
hed_gui_render_label(
	struct hed_gui_s* ctx,
	hed_ref_str_t label,
	const hed_label_opts_t* options
) {
	return ctx->render_label(ctx, label, options);
}

static inline bool
hed_gui_begin_popup(struct hed_gui_s* ctx, hed_ref_str_t label) {
	return ctx->begin_popup(ctx, label);
}

static inline void
hed_gui_end_popup(struct hed_gui_s* ctx) {
	ctx->end_popup(ctx);
}

#endif
