#include "common.h"
#include "plugin1.h"
#include "plugin2.h"
#include <stdlib.h>

fixture_t*
create_fixture(void) {
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
	free(builder_mem);

	mem_required = 0;
	hgraph_config_t graph_config = {
		.max_nodes = 32,
		.max_name_length = 64,
	};
	hgraph_init(registry, &graph_config, NULL, &mem_required);
	void* graph_mem = malloc(mem_required);
	hgraph_t* graph = hgraph_init(registry, &graph_config, graph_mem, &mem_required);

	fixture_t* fixture = malloc(sizeof(fixture_t));
	*fixture = (fixture_t){
		.graph = graph,
		.registry = registry,
	};

	return fixture;
}

void
destroy_fixture(fixture_t* fixture) {
	free(fixture->graph);
	free(fixture->registry);
	free(fixture);
}
