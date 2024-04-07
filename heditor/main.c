#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>

static struct {
    sg_pass_action pass_action;
} state;

static
void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    // initial clear color
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.0f, 0.5f, 1.0f, 1.0 } }
    };
}

static void
frame(void) {
    sg_begin_pass(&(sg_pass){
		.action = state.pass_action,
		.swapchain = sglue_swapchain()
	});
    sg_end_pass();
    sg_commit();
}

static void
cleanup(void) {
    sg_shutdown();
}

static void
event(const sapp_event* ev) {
	(void)ev;
}

sapp_desc
sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .window_title = "Hello Sokol + Dear ImGui",
        .width = 800,
        .height = 600,
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
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
