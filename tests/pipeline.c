#include "hgraph/runtime.h"
#include "rktest.h"
#include "plugin1.h"
#include "plugin2.h"
#include "common.h"
#include <stdlib.h>

static struct {
	fixture_t* base;
	hgraph_pipeline_t* pipeline;
	void* pipeline_mem;
} fixture;

TEST_SETUP(pipeline) {
	fixture.base = create_fixture();
	hgraph_t* graph = fixture.base->graph;

	// Create the following:
	// |start| -> |mid| -> |end|
	hgraph_index_t start = hgraph_create_node(
		graph, &plugin1_start
	);
	hgraph_set_node_name(graph, start, HGRAPH_STR("start"));

	hgraph_index_t mid = hgraph_create_node(
		graph, &plugin2_mid
	);
	hgraph_set_node_name(graph, mid, HGRAPH_STR("mid"));

	hgraph_index_t end = hgraph_create_node(
		graph, &plugin1_end
	);
	hgraph_set_node_name(graph, end, HGRAPH_STR("end"));

	hgraph_connect(
		graph,
		hgraph_get_pin_id(graph, start, &plugin1_start_out_f32),
		hgraph_get_pin_id(graph, mid, &plugin2_mid_in_f32)
	);
	hgraph_connect(
		graph,
		hgraph_get_pin_id(graph, mid, &plugin2_mid_out_i32),
		hgraph_get_pin_id(graph, end, &plugin1_end_in_i32)
	);

	size_t mem_required = 0;
	hgraph_pipeline_init(
		graph,
		&(hgraph_pipeline_config_t){
			.max_scratch_memory = 4096
		},
		NULL,
		NULL,
		&mem_required
	);

	fixture.pipeline_mem = malloc(mem_required);
	fixture.pipeline = hgraph_pipeline_init(
		graph,
		&(hgraph_pipeline_config_t){
			.max_scratch_memory = 4096
		},
		NULL,
		fixture.pipeline_mem,
		&mem_required
	);
}

TEST_TEARDOWN(pipeline) {
	free(fixture.pipeline_mem);
	destroy_fixture(fixture.base);
}

TEST(pipeline, execute) {
	hgraph_pipeline_t* pipeline = fixture.pipeline;
	hgraph_t* graph = fixture.base->graph;

	hgraph_index_t start = hgraph_get_node_by_name(graph, HGRAPH_STR("start"));
	hgraph_index_t mid = hgraph_get_node_by_name(graph, HGRAPH_STR("mid"));
	hgraph_index_t end = hgraph_get_node_by_name(graph, HGRAPH_STR("end"));
	ASSERT_TRUE(HGRAPH_IS_VALID_INDEX(start));
	ASSERT_TRUE(HGRAPH_IS_VALID_INDEX(mid));
	ASSERT_TRUE(HGRAPH_IS_VALID_INDEX(end));

	hgraph_set_node_attribute(graph, start, &plugin1_start_attr_f32, &(float){ 4.20f });
	{
		hgraph_pipeline_execution_status_t status = hgraph_pipeline_execute(pipeline, NULL, NULL);
		ASSERT_EQ(status, HGRAPH_PIPELINE_EXEC_FINISHED);

		const int32_t* result = (const int32_t*)hgraph_pipeline_get_node_status(pipeline, end);
		ASSERT_TRUE(result != NULL);
		ASSERT_EQ(*result, 5);
	}

	hgraph_set_node_attribute(graph, mid, &plugin2_mid_attr_round_up, &(bool){ false });
	{
		hgraph_pipeline_execution_status_t status = hgraph_pipeline_execute(pipeline, NULL, NULL);
		ASSERT_EQ(status, HGRAPH_PIPELINE_EXEC_FINISHED);

		const int32_t* result = (const int32_t*)hgraph_pipeline_get_node_status(pipeline, end);
		ASSERT_TRUE(result != NULL);
		ASSERT_EQ(*result, 4);
	}
}

TEST(pipeline, transfer) {
	hgraph_pipeline_t* pipeline = fixture.pipeline;
	hgraph_t* graph = fixture.base->graph;

	hgraph_index_t start = hgraph_get_node_by_name(graph, HGRAPH_STR("start"));
	hgraph_index_t mid = hgraph_get_node_by_name(graph, HGRAPH_STR("mid"));
	hgraph_index_t end = hgraph_get_node_by_name(graph, HGRAPH_STR("end"));

	// Execute once so mid now has some data
	hgraph_set_node_attribute(graph, start, &plugin1_start_attr_f32, &(float){ 4.20f });
	hgraph_pipeline_execution_status_t status = hgraph_pipeline_execute(pipeline, NULL, NULL);
	ASSERT_EQ(status, HGRAPH_PIPELINE_EXEC_FINISHED);

	// Delete a node so the pipeline is out of sync
	hgraph_destroy_node(graph, end);
	status = hgraph_pipeline_execute(pipeline, NULL, NULL);
	ASSERT_EQ(status, HGRAPH_PIPELINE_EXEC_OUT_OF_SYNC);

	// Create a new pipeline
	size_t mem_required = 0;
	hgraph_pipeline_init(
		graph,
		&(hgraph_pipeline_config_t){
			.max_scratch_memory = 4096
		},
		NULL,
		NULL,
		&mem_required
	);

	// Transfer data from the old pipeline
	void* new_pipeline_mem = malloc(mem_required);
	hgraph_pipeline_t* new_pipeline = hgraph_pipeline_init(
		graph,
		&(hgraph_pipeline_config_t){
			.max_scratch_memory = 4096
		},
		pipeline,
		new_pipeline_mem,
		&mem_required
	);

	status = hgraph_pipeline_execute(new_pipeline, NULL, NULL);
	ASSERT_EQ(status, HGRAPH_PIPELINE_EXEC_FINISHED);

	const mid_state_t* mid_state = hgraph_pipeline_get_node_status(new_pipeline, mid);
	// Should be 2 even though this instance is only executed once
	ASSERT_EQ(mid_state->num_executions, 2);

	free(new_pipeline_mem);
}
