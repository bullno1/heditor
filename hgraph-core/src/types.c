#include "hgraph/common.h"
#include <hgraph/core.h>
#include <hgraph/io.h>
#include <heditor/plugin.h>
#include <float.h>
#include <stdint.h>

static inline hgraph_io_status_t
hgraph_core_serialize_f32(const void* value, hgraph_out_t* out) {
	return hgraph_io_write_f32(*(const float*)value, out);
}

static inline hgraph_io_status_t
hgraph_core_deserialize_f32(void* value, hgraph_in_t* in) {
	return hgraph_io_read_f32((float*)value, in);
}

static inline void
hgraph_core_render_f32(void* value, void* gui) {
	hed_gui_render_f32_input(
		gui,
		value,
		&(hed_scalar_input_opts_t){
			.type = HED_SCALAR_INPUT_TEXT_BOX,
			.min.f32 = -FLT_MAX,
			.max.f32 = FLT_MAX,
			.step.f32 = 1.f,
		}
	);
}

static inline hgraph_io_status_t
hgraph_core_serialize_i32(const void* value, hgraph_out_t* out) {
	int64_t tmp = *(const int32_t*)value;
	return hgraph_io_write_sint(tmp, out);
}

static inline hgraph_io_status_t
hgraph_core_deserialize_i32(void* value, hgraph_in_t* in) {
	int64_t tmp;
	HGRAPH_CHECK_IO(hgraph_io_read_sint(&tmp, in));
	if (INT32_MIN <= tmp && tmp <= INT32_MAX) {
		*(int32_t*)value = (int32_t)tmp;
		return HGRAPH_IO_OK;
	} else {
		return HGRAPH_IO_MALFORMED;
	}
}

static inline void
hgraph_core_render_i32(void* value, void* gui) {
	hed_gui_render_i32_input(
		gui,
		value,
		&(hed_scalar_input_opts_t){
			.type = HED_SCALAR_INPUT_TEXT_BOX,
			.min.i32 = INT32_MIN,
			.max.i32 = INT32_MAX,
			.step.i32 = 1,
		}
	);
}

static inline hgraph_io_status_t
hgraph_core_serialize_fixed_str(const void* value, hgraph_out_t* out) {
	const hgraph_core_fixed_str_t* tmp = value;
	return hgraph_io_write_str(
		(hgraph_str_t){ .length = tmp->len, .data = tmp->data },
		out
	);
}

static inline hgraph_io_status_t
hgraph_core_deserialize_fixed_str(void* value, hgraph_in_t* in) {
	hgraph_core_fixed_str_t* fixed_str = value;
	fixed_str->len = HGRAPH_CORE_FIXED_STR_LEN;
	return hgraph_io_read_str(fixed_str->data, &fixed_str->len, in);
}

static inline void
hgraph_core_render_fixed_str(void* value, void* gui) {
	hgraph_core_fixed_str_t* fixed_str = value;
	hed_editable_str_t gui_str = {
		.capacity = HGRAPH_CORE_FIXED_STR_LEN,
		.len = fixed_str->len,
		.chars = fixed_str->data,
	};
	bool changed = hed_gui_render_string_input(
		gui,
		&gui_str,
		&(hed_text_input_opts_t){ .type = HED_TEXT_INPUT_AREA }
	);
	if (changed) {
		fixed_str->len = gui_str.len;
	}
}

static inline hgraph_io_status_t
hgraph_core_serialize_var_str(const void* value, hgraph_out_t* out) {
	(void)value;
	return hgraph_io_write_str(HGRAPH_STR(""), out);
}

static inline hgraph_io_status_t
hgraph_core_deserialize_var_str(void* value, hgraph_in_t* in) {
	(void)value;
	char buf;
	size_t len = 0;
	return hgraph_io_read_str(&buf, &len, in);
}

const hgraph_data_type_t hgraph_core_f32 = {
	.name = HGRAPH_STR("core.f32"),
	.label = HGRAPH_STR("f32"),
	.description = HGRAPH_STR("Floating point"),
	.size = sizeof(float),
	.alignment = _Alignof(float),
	.serialize = hgraph_core_serialize_f32,
	.deserialize = hgraph_core_deserialize_f32,
	.render = hgraph_core_render_f32,
};

const hgraph_data_type_t hgraph_core_i32 = {
	.name = HGRAPH_STR("core.i32"),
	.label = HGRAPH_STR("i32"),
	.description = HGRAPH_STR("Integer"),
	.size = sizeof(int32_t),
	.alignment = _Alignof(int32_t),
	.serialize = hgraph_core_serialize_i32,
	.deserialize = hgraph_core_deserialize_i32,
	.render = hgraph_core_render_i32,
};

const hgraph_data_type_t hgraph_core_fixed_str = {
	.name = HGRAPH_STR("core.fixed_str"),
	.label = HGRAPH_STR("str"),
	.description = HGRAPH_STR("Short string"),
	.size = sizeof(hgraph_core_fixed_str_t),
	.alignment = _Alignof(hgraph_core_fixed_str_t),
	.serialize = hgraph_core_serialize_fixed_str,
	.deserialize = hgraph_core_deserialize_fixed_str,
	.render = hgraph_core_render_fixed_str,
};

const hgraph_data_type_t hgraph_core_var_str = {
	.name = HGRAPH_STR("core.var_str"),
	.label = HGRAPH_STR("str"),
	.description = HGRAPH_STR("Long string"),
	.size = sizeof(hgraph_str_t),
	.alignment = _Alignof(hgraph_str_t),
	.serialize = hgraph_core_serialize_var_str,
	.deserialize = hgraph_core_deserialize_var_str,
};
