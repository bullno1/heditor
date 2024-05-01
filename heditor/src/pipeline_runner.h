#ifndef HEDITOR_PIPELINE_RUNNER_H
#define HEDITOR_PIPELINE_RUNNER_H

#include "hthread.h"
#include <hgraph/runtime.h>

typedef enum {
	PIPELINE_RUNNING,
	PIPELINE_PAUSED,
	PIPELINE_STOPPED,
} pipeline_runner_state_t;

typedef struct {
	hed_spsc_queue_t cmd_queue;
	void* cmd_buf[2];
	atomic_int current_state;
	atomic_int execution_status;
	atomic_bool should_run;
	thrd_t thread;
} pipeline_runner_t;

void
pipeline_runner_init(pipeline_runner_t* runner);

void
pipeline_runner_execute(pipeline_runner_t* runner, hgraph_pipeline_t* pipeline);

pipeline_runner_state_t
pipeline_runner_current_state(pipeline_runner_t* runner);

hgraph_pipeline_execution_status_t
pipeline_runner_execution_status(pipeline_runner_t* runner);

void
pipeline_runner_pause(pipeline_runner_t* runner);

void
pipeline_runner_resume(pipeline_runner_t* runner);

void
pipeline_runner_stop(pipeline_runner_t* runner);

void
pipeline_runner_terminate(pipeline_runner_t* runner);

#endif
