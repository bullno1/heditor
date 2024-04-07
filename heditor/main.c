#include <remodule.h>
#include <remodule_monitor.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>

#ifndef NDEBUG
#	define HEDITOR_LIB_SUFFIX "_d"
#else
#	define HEDITOR_LIB_SUFFIX ""
#endif

static remodule_t* app_module = NULL;
static remodule_monitor_t* app_monitor = NULL;
static sapp_desc app = { 0 };

static
void init(void) {
	if (app.init_cb) {
		app.init_cb();
	} else if (app.init_userdata_cb) {
		app.init_userdata_cb(app.user_data);
	}
}

static void
cleanup(void) {
	if (app.cleanup_cb) {
		app.cleanup_cb();
	} else if (app.cleanup_userdata_cb) {
		app.cleanup_userdata_cb(app.user_data);
	}

	remodule_unmonitor(app_monitor);
	remodule_unload(app_module);
}

static void
frame(void) {
	remodule_check(app_monitor);

	if (app.frame_cb) {
		app.frame_cb();
	} else if (app.frame_userdata_cb) {
		app.frame_userdata_cb(app.user_data);
	}
}

static void
event(const sapp_event* ev) {
	if (app.event_cb) {
		app.event_cb(ev);
	} else if (app.event_userdata_cb) {
		app.event_userdata_cb(ev, app.user_data);
	}
}

sapp_desc
sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

	app_module = remodule_load(
		"heditor_app" HEDITOR_LIB_SUFFIX REMODULE_DYNLIB_EXT,
		&app
	);
	app_monitor = remodule_monitor(app_module);

	sapp_desc desc = app;

	// All callbacks have to be hooked and forwarded since sokol will make
	// a copy of this structure.
	// Futher modifications by the plugin will not be recognized by sokol.
	desc.init_cb = init;
	desc.frame_cb = frame;
	desc.event_cb = event;
	desc.cleanup_cb = cleanup;

	return desc;
}

#if defined(__has_feature)
#	if __has_feature(address_sanitizer)
#		define HAS_ASAN 1
#	else
#		define HAS_ASAN 0
#	endif
#elif defined(__SANITIZE_ADDRESS__)
#	define HAS_ASAN 1
#else
#	define HAS_ASAN 0
#endif

#ifdef HAS_ASAN

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
