#include "plugin1.h"
#include "plugin2.h"
#include <munit.h>
#include <hgraph/runtime.h>

static MunitResult
manipulation(const MunitParameter params[], void* fixture) {
	(void)params;
	(void)fixture;

	hgraph_registry_config_t reg_config = {
		.max_data_types = 32,
		.max_node_types = 32,
	};
	size_t mem_required = 0;
	hgraph_registry_builder_init(&reg_config, NULL, &mem_required);
	void* builder_mem = malloc(mem_required);
	hgraph_registry_builder_t* builder = hgraph_registry_builder_init(
		&reg_config,
		builder_mem,
		&mem_required
	);

	hgraph_plugin_api_t* plugin_api = hgraph_registry_builder_as_plugin_api(builder);
	plugin1_entry(plugin_api);
	plugin2_entry(plugin_api);

	mem_required = 0;
	hgraph_registry_init(builder, NULL, &mem_required);
	void* registry_mem = malloc(mem_required);
	hgraph_registry_t* registry = hgraph_registry_init(builder, registry_mem, &mem_required);

	mem_required = 0;
	hgraph_config_t graph_config = {
		.max_nodes = 32,
		.max_name_length = 64,
	};
	hgraph_init(registry, &graph_config, NULL, &mem_required);
	void* graph_mem = malloc(mem_required);
	hgraph_t* graph = hgraph_init(registry, &graph_config, graph_mem, &mem_required);
	munit_assert_not_null(graph);

	free(graph_mem);
	free(registry_mem);
	free(builder_mem);
	return MUNIT_OK;
}

MunitSuite test_graph = {
	.prefix = "/graph",
	.tests = (MunitTest[]){
		{
			.name = "/manipulation",
			.test = manipulation,
		},
		{ 0 },
	},
};
