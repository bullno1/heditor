#include "internal.h"
#include "mem_layout.h"

size_t
hgraph_pipeline_init(
	hgraph_pipeline_t* pipeline,
	const hgraph_pipeline_config_t* config
) {
	mem_layout_t layout = { 0 };
}

void
hgraph_pipeline_cleanup(hgraph_pipeline_t* pipeline);

hgraph_pipeline_execution_status_t
hgraph_pipeline_execute(
	hgraph_pipeline_t* pipeline,
	hgraph_pipeline_watcher_t watcher,
	void* userdata
);

const void*
hgraph_pipeline_get_node_status(
	hgraph_pipeline_t* pipeline,
	hgraph_index_t node
);

hgraph_pipeline_stats_t
hgraph_pipeline_get_stats(hgraph_pipeline_t* pipeline);
