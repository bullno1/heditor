#include "allocator/arena.h"
#include "resources.h"
#include "qoi.h"
#include <xincbin.h>
#include <sokol_app.h>

sapp_icon_desc
load_app_icon(hed_arena_t* arena) {
	xincbin_data_t qoi_icon = XINCBIN_GET(icon);
	struct qoidecoder decoder = qoidecoder(qoi_icon.data, (int)qoi_icon.size);
	if (decoder.error) {
		return (sapp_icon_desc){ .sokol_default = true };
	}

	int num_pixels = decoder.count;
	size_t icon_size = sizeof(unsigned) * num_pixels;
	unsigned* pixels = hed_arena_alloc(arena, icon_size, _Alignof(unsigned));
	for (int i = 0; i < num_pixels; ++i) {
		pixels[i] = qoidecode(&decoder);
	}

	if (decoder.error) {
		return (sapp_icon_desc){ .sokol_default = true };
	} else {
		return (sapp_icon_desc){
			.images = {
				{
					.width = decoder.width,
					.height = decoder.height,
					.pixels = {
						.ptr = pixels,
						.size = icon_size,
					},
				},
			},
		};
	}
}
