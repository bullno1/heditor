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

#ifdef REMODULE_API
#pragma message #REMODULE_API
#endif

#include <remodule.h>
#include <remodule_monitor.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include "pico_log.h"
