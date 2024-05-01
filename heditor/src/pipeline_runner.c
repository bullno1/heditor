#include "pipeline_runner.h"
#include "hthread.h"
#include <stdbool.h>
#include <threads.h>
#include <hgraph/runtime.h>

static char pipeline_cmd_stop;
static char pipeline_cmd_resume;
static char pipeline_cmd_pause;
static char pipeline_cmd_terminate;

static bool
pipeline_watcher(const hgraph_pipeline_event_t* event, void* userdata) {
	(void)event;
	pipeline_runner_t* runner = userdata;

	while (runner->should_run) {
		void* cmd = hed_spsc_queue_consume(&runner->cmd_queue, 0);
		if (cmd == NULL) {
			return true;
		} else if (cmd == &pipeline_cmd_stop) {
			return false;
		} else if (cmd == &pipeline_cmd_pause) {
			while (runner->should_run) {
				void* cmd = hed_spsc_queue_consume(&runner->cmd_queue, -1);
				if (cmd == &pipeline_cmd_resume) {
					break;
				}
			}
		} else if (cmd == &pipeline_cmd_terminate) {
			return false;
		}
	}

	return false;
}

static int
pipeline_runner_entry(void* args) {
	pipeline_runner_t* runner = args;

	while (runner->should_run) {
		void* cmd = hed_spsc_queue_consume(&runner->cmd_queue, -1);

		if (cmd == &pipeline_cmd_terminate) {
			break;
		} else if (
			cmd == &pipeline_cmd_stop
			|| cmd == &pipeline_cmd_resume
			|| cmd == &pipeline_cmd_pause
		) {
			continue;
		} else {
			hgraph_pipeline_t* pipeline = cmd;
			atomic_store(&runner->current_state, PIPELINE_RUNNING);
			hgraph_pipeline_execution_status_t status = hgraph_pipeline_execute(
				pipeline, pipeline_watcher, runner
			);
			atomic_store(&runner->current_state, PIPELINE_STOPPED);
			atomic_store(&runner->execution_status, status);
		}
	}

	return thrd_success;
}

void
pipeline_runner_init(pipeline_runner_t* runner) {
	hed_spsc_queue_init(
		&runner->cmd_queue,
		sizeof(runner->cmd_buf) / sizeof(runner->cmd_buf[0]),
		runner->cmd_buf,
		0
	);
	runner->current_state = PIPELINE_STOPPED;
	runner->execution_status = HGRAPH_PIPELINE_EXEC_FINISHED;
	runner->should_run = true;
	thrd_create(&runner->thread, pipeline_runner_entry, runner);
}

void
pipeline_runner_terminate(pipeline_runner_t* runner) {
	runner->should_run = false;
	hed_spsc_queue_produce(&runner->cmd_queue, &pipeline_cmd_terminate, -1);
	thrd_join(runner->thread, NULL);
	hed_spsc_queue_cleanup(&runner->cmd_queue);
}

void
pipeline_runner_execute(pipeline_runner_t* runner, hgraph_pipeline_t* pipeline) {
	if (runner->current_state == PIPELINE_STOPPED) {
		hed_spsc_queue_produce(&runner->cmd_queue, pipeline, 0);
	}
}

pipeline_runner_state_t
pipeline_runner_current_state(pipeline_runner_t* runner) {
	return runner->current_state;
}

hgraph_pipeline_execution_status_t
pipeline_runner_execution_status(pipeline_runner_t* runner) {
	return runner->execution_status;
}

void
pipeline_runner_pause(pipeline_runner_t* runner) {
	hed_spsc_queue_produce(&runner->cmd_queue, &pipeline_cmd_pause, 0);
}

void
pipeline_runner_resume(pipeline_runner_t* runner) {
	hed_spsc_queue_produce(&runner->cmd_queue, &pipeline_cmd_resume, 0);
}

void
pipeline_runner_stop(pipeline_runner_t* runner) {
	hed_spsc_queue_produce(&runner->cmd_queue, &pipeline_cmd_stop, 0);
}
