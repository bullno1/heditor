#include "data.h"

const hgraph_data_type_t test_f32 = {
	.size = sizeof(float),
	.alignment = _Alignof(float),
	.name = HGRAPH_STR("f32"),
};

const hgraph_data_type_t test_i32 = {
	.size = sizeof(int32_t),
	.alignment = _Alignof(int32_t),
	.name = HGRAPH_STR("i32"),
};
