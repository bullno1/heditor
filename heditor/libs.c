#define SOKOL_IMPL
#if defined(__MINGW32__)
#define SOKOL_GLCORE33
#elif defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
#define SOKOL_GLES3
#elif defined(__APPLE__)
#define SOKOL_METAL
#else
#define SOKOL_GLCORE33
#endif

#define _DEFAULT_SOURCE 1

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>

#ifndef NDEBUG

const char*
__lsan_default_suppressions(void) {
	// NVidia driver always leak
	return "leak:_sapp_glx_create_context";
}

const char*
__asan_default_options(void) {
	return "fast_unwind_on_malloc=0";
}

#endif
