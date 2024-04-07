#define REMODULE_PLUGIN_IMPLEMENTATION
#include <remodule.h>
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
}

static void
event(const sapp_event* ev) {
	(void)ev;
}

static void
frame(void) {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.5f, 0.5f, 0.5f, 1.0 } }
    };

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

void
remodule_entry(remodule_op_t op, void* userdata) {
	sapp_desc* app = userdata;
	if (op == REMODULE_OP_LOAD || op == REMODULE_OP_AFTER_RELOAD) {
		*app = (sapp_desc){
			.init_cb = init,
			.cleanup_cb = cleanup,
			.event_cb = event,
			.frame_cb = frame,
			.window_title = "heditor",
			.width = 1280,
			.height = 720,
			.icon.sokol_default = true,
			.logger.func = slog_func,
		};
	}
}
