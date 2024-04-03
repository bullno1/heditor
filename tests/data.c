#include "data.h"
#include "hgraph/common.h"
#include <stdbool.h>
#include <hgraph/io.h>
#include <stdint.h>

static hgraph_io_status_t
write_f32(const void* value, hgraph_out_t* out) {
	float f32 = *(float*)value;
	return hgraph_io_write_f32(f32, out);
}

static hgraph_io_status_t
read_f32(void* value, hgraph_in_t* in) {
	return hgraph_io_read_f32(value, in);
}

static hgraph_io_status_t
write_i32(const void* value, hgraph_out_t* out) {
	float i32 = *(int32_t*)value;
	return hgraph_io_write_sint(i32, out);
}

static hgraph_io_status_t
read_i32(void* value, hgraph_in_t* in) {
	int64_t sint;
	HGRAPH_CHECK_IO(hgraph_io_read_sint(&sint, in));
	if (INT32_MIN <= sint && sint <= INT32_MAX) {
		*(int32_t*)value = sint;
		return HGRAPH_IO_OK;
	} else {
		return HGRAPH_IO_MALFORMED;
	}
}

static const char bool_true = '1';
static const char bool_false = '0';

static hgraph_io_status_t
write_bool(const void* value, hgraph_out_t* out) {
	bool bool_value = *(bool*)value;
	return bool_value
		? hgraph_io_write(out, &bool_true, sizeof(bool))
		: hgraph_io_write(out, &bool_false, sizeof(bool));
}

static hgraph_io_status_t
read_bool(void* value, hgraph_in_t* in) {
	char read_value;
	bool* out_value = value;
	HGRAPH_CHECK_IO(hgraph_io_read(in, &read_value, sizeof(read_value)));
	if (read_value == bool_true) {
		*out_value = true;
		return HGRAPH_IO_OK;
	} else if (read_value == bool_false) {
		*out_value = false;
		return HGRAPH_IO_OK;
	} else {
		return HGRAPH_IO_MALFORMED;
	}
}

const hgraph_data_type_t test_f32 = {
	.size = sizeof(float),
	.alignment = _Alignof(float),
	.name = HGRAPH_STR("f32"),
	.serialize = write_f32,
	.deserialize = read_f32,
};

const hgraph_data_type_t test_i32 = {
	.size = sizeof(int32_t),
	.alignment = _Alignof(int32_t),
	.name = HGRAPH_STR("i32"),
	.serialize = write_i32,
	.deserialize = read_i32,
};

const hgraph_data_type_t test_bool = {
	.size = sizeof(bool),
	.alignment = _Alignof(bool),
	.name = HGRAPH_STR("bool"),
	.serialize = write_bool,
	.deserialize = read_bool,
};
