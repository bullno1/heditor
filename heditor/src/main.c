#include <remodule.h>
#include <remodule_monitor.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include "pico_log.h"
#include "entry.h"
#include "allocator/std.h"
#include "allocator/sokol.h"
#include "allocator/counting.h"

#ifndef NDEBUG
#	define HEDITOR_LIB_SUFFIX "_d"
#else
#	define HEDITOR_LIB_SUFFIX ""
#endif

static remodule_t* app_module = NULL;
static remodule_monitor_t* app_monitor = NULL;
static entry_args_t entry = { 0 };
static hed_allocator_t allocator = { 0 };
static bool should_reload = false;

#ifndef NDEBUG
static hed_counting_allocator_t app_allocator = { 0 };
static hed_counting_allocator_t sokol_allocator = { 0 };
#endif

static
void init(void) {
	if (entry.app.init_cb) {
		entry.app.init_cb();
	} else if (entry.app.init_userdata_cb) {
		entry.app.init_userdata_cb(entry.app.user_data);
	}
}

static void
cleanup(void) {
	if (entry.app.cleanup_cb) {
		entry.app.cleanup_cb();
	} else if (entry.app.cleanup_userdata_cb) {
		entry.app.cleanup_userdata_cb(entry.app.user_data);
	}

	remodule_unmonitor(app_monitor);
	remodule_unload(app_module);

#ifndef NDEBUG
	log_debug("Peak app allocation %zu", app_allocator.peak_allocated);
	log_debug("Peak sokol allocation %zu", sokol_allocator.peak_allocated);
#endif
}

static void
frame(void) {
	should_reload = remodule_should_reload(app_monitor) || should_reload;
	if (should_reload && !entry.reload_blocked) {
		remodule_reload(app_module);
		should_reload = false;

		log_info("Reloaded app");
	}

	if (entry.app.frame_cb) {
		entry.app.frame_cb();
	} else if (entry.app.frame_userdata_cb) {
		entry.app.frame_userdata_cb(entry.app.user_data);
	}
}

static void
event(const sapp_event* ev) {
	if (entry.app.event_cb) {
		entry.app.event_cb(ev);
	} else if (entry.app.event_userdata_cb) {
		entry.app.event_userdata_cb(ev, entry.app.user_data);
	}
}

static void
sokol_log(
	const char* tag,                // always "sapp"
	uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
	uint32_t log_item_id,           // SAPP_LOGITEM_*
	const char* message_or_null,    // a message string, may be nullptr in release mode
	uint32_t line_nr,               // line number in sokol_app.h
	const char* filename_or_null,   // source filename, may be nullptr in release mode
	void* user_data
) {
	(void)user_data;

	log_level_t level;
	switch (log_level) {
		case 0:
			level = LOG_LEVEL_FATAL;
			break;
		case 1:
			level = LOG_LEVEL_ERROR;
			break;
		case 2:
			level = LOG_LEVEL_WARN;
			break;
		case 3:
			level = LOG_LEVEL_INFO;
			break;
		default:
			level = LOG_LEVEL_INFO;
			break;
	}

	log_write(
		level,
		filename_or_null != NULL ? filename_or_null : "<unknown>",
		line_nr,
		"<unknown>",
		"%s(%d): %s",
		tag, log_item_id, message_or_null != NULL ? message_or_null : "<empty>"
	);
}

sapp_desc
sokol_main(int argc, char* argv[]) {
	// Args
	entry.argc = argc;
	entry.argv = argv;

	// Allocator
	allocator = hed_std_allocator();
#ifndef NDEBUG
	entry.allocator = hed_counting_allocator_init(&app_allocator, &allocator);
	entry.sokol_allocator = hed_allocator_to_sokol(
		hed_counting_allocator_init(&sokol_allocator, &allocator)
	);
#else
	entry.allocator = &allocator;
	entry.sokol_allocator = hed_allocator_to_sokol(&allocator);
#endif

	// Setup logging
	log_appender_t id = log_add_stream(stderr, LOG_LEVEL_TRACE);
	log_set_time_fmt(id, "%H:%M:%S");
	log_display_colors(id, true);
	log_display_timestamp(id, true);
	log_display_file(id, true);
	entry.sokol_logger.func = sokol_log;

	// Load main app module
	app_module = remodule_load(
		"heditor_app" HEDITOR_LIB_SUFFIX REMODULE_DYNLIB_EXT,
		&entry
	);
	app_monitor = remodule_monitor(app_module);

	sapp_desc desc = entry.app;

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
	return
		"leak:_sapp_glx_create_context\n"  // Nvidia driver
		"leak:libfontconfig\n"
		"leak:libglib-2.0\n"
		"leak:libdbus-1\n";
}

#endif
