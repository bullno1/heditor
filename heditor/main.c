#include <remodule.h>
#include <remodule_monitor.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include "pico_log.h"
#include "entry.h"

#ifndef NDEBUG
#	define HEDITOR_LIB_SUFFIX "_d"
#else
#	define HEDITOR_LIB_SUFFIX ""
#endif

static remodule_t* app_module = NULL;
static remodule_monitor_t* app_monitor = NULL;
static entry_args_t entry = { 0 };

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
}

static void
frame(void) {
	if (remodule_check(app_monitor)) {
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
log(
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
	entry.argc = argc;
	entry.argv = argv;

	// Setup logging
	log_appender_t id = log_add_stream(stderr, LOG_LEVEL_TRACE);
	log_set_time_fmt(id, "%H:%M:%S");
	log_display_colors(id, true);
	log_display_timestamp(id, true);
	log_display_file(id, true);
	entry.logger.func = log;

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
	desc.logger.func = log;

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
