#ifndef HEDITOR_PLUGIN_H
#define HEDITOR_PLUGIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum hed_gui_scalar_type_e {
	HED_GUI_SCALAR_FLOAT,
	HED_GUI_SCALAR_INT,
} hed_gui_scalar_type_t;

typedef union hed_gui_scalar_value_u {
	float f32;
	int32_t i32;
} hed_gui_scalar_value_t;

typedef struct hed_gui_scalar_s {
	hed_gui_scalar_type_t type;
	hed_gui_scalar_value_t value;
} hed_gui_scalar_t;

typedef enum hed_scalar_input_type_e {
	HED_SCALAR_INPUT_SLIDER,
	HED_SCALAR_INPUT_TEXT,
	HED_SCALAR_INPUT_LABEL,
} hed_scalar_input_type_t;

typedef struct hed_scalar_input_opts_s {
	hed_scalar_input_type_t type;
	hed_gui_scalar_value_t min;
	hed_gui_scalar_value_t max;
	hed_gui_scalar_value_t step;
} hed_scalar_input_opts_t;

typedef enum hed_text_input_type_e {
	HED_TEXT_INPUT_LINE,
	HED_TEXT_INPUT_AREA,
	HED_TEXT_INPUT_FILE_PICKER,
} hed_text_input_type_t;

typedef struct hed_text_input_opts_s {
	hed_text_input_type_t type;
} hed_text_input_opts_t;

typedef struct hed_gui_str_s {
	size_t capacity;
	size_t len;
	char* storage;
} hed_gui_str_t;

typedef struct hed_gui_s {
	bool (*render_scalar)(
		struct hed_gui_s* ctx,
		hed_gui_scalar_t* value,
		const hed_scalar_input_opts_t* options
	);
	bool (*render_text)(
		struct hed_gui_s* ctx,
		hed_gui_str_t* value,
		const hed_text_input_opts_t* options
	);
	bool (*render_enum)(
		struct hed_gui_s* ctx,
		int* value,
		int count,
		const char* labels
	);

	void* imgui_ctx;
} hed_gui_t;

static inline bool
hed_gui_render_scalar(
	struct hed_gui_s* ctx,
	hed_gui_scalar_t* value,
	const hed_scalar_input_opts_t* options
) {
	return ctx->render_scalar(ctx, value, options);
}

static inline bool
hed_gui_render_f32(
	struct hed_gui_s* ctx,
	float* value,
	const hed_scalar_input_opts_t* options
) {
	hed_gui_scalar_t tmp = {
		.type = HED_GUI_SCALAR_FLOAT,
		.value.f32 = *value,
	};
	bool result = hed_gui_render_scalar(ctx, &tmp, options);
	if (result) { *value = tmp.value.f32; }
	return result;
}

static inline bool
hed_gui_render_i32(
	struct hed_gui_s* ctx,
	int32_t* value,
	const hed_scalar_input_opts_t* options
) {
	hed_gui_scalar_t tmp = {
		.type = HED_GUI_SCALAR_INT,
		.value.i32 = *value,
	};
	bool result = hed_gui_render_scalar(ctx, &tmp, options);
	if (result) { *value = tmp.value.i32; }
	return result;
}

static inline bool
hed_gui_render_text(
	struct hed_gui_s* ctx,
	hed_gui_str_t* value,
	const hed_text_input_opts_t* options
) {
	return ctx->render_text(ctx, value, options);
}

static inline bool
hed_gui_render_enum(
	struct hed_gui_s* ctx,
	int* value,
	int count,
	const char* labels
) {
	return ctx->render_enum(ctx, value, count, labels);
}

#endif
