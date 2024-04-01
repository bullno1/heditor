#include "hgraph/runtime.h"
#include "rktest.h"
#include "common.h"
#include "plugin1.h"
#include <stdlib.h>

static struct {
	fixture_t base;
	hgraph_registry_builder_t* builder;
} fixture;

TEST_SETUP(migration) {
	fixture_init(&fixture.base);
	create_start_mid_end_graph(fixture.base.graph);

	hgraph_registry_config_t registry_config = {
		.max_data_types = 64,
		.max_node_types = 64,
	};

	size_t mem_size = hgraph_registry_builder_init(NULL, &registry_config);
	fixture.builder = malloc(mem_size);

	hgraph_registry_builder_init(fixture.builder, &registry_config);
}

TEST_TEARDOWN(migration) {
	free(fixture.builder);
	fixture_cleanup(&fixture.base);
}

TEST(migration, missing_type) {
	hgraph_registry_builder_t* builder = fixture.builder;
	plugin1_entry(
		hgraph_registry_builder_as_plugin_api(builder)
	);
	size_t reg_size = hgraph_registry_init(NULL, builder);
	hgraph_registry_t* new_reg = malloc(reg_size);
	hgraph_registry_init(new_reg, builder);

	size_t migration_size = hgraph_migration_init(NULL, fixture.base.registry, new_reg);
	hgraph_migration_t* migration = malloc(migration_size);
	hgraph_migration_init(migration, fixture.base.registry, new_reg);

	hgraph_config_t new_graph_config = {
		.registry = new_reg,
		.max_nodes = 64,
		.max_name_length = 64,
	};
	size_t new_graph_size = hgraph_init(NULL, &new_graph_config);
	hgraph_t* new_graph = malloc(new_graph_size);
	hgraph_init(new_graph, &new_graph_config);

	hgraph_migration_execute(migration, fixture.base.graph, new_graph);

	hgraph_info_t old_info = hgraph_get_info(fixture.base.graph);
	ASSERT_EQ(old_info.num_nodes, 3);
	ASSERT_EQ(old_info.num_edges, 2);

	hgraph_info_t new_info = hgraph_get_info(new_graph);
	ASSERT_EQ(new_info.num_nodes, 2);
	ASSERT_EQ(new_info.num_edges, 0);

	free(new_graph);
	free(migration);
	free(new_reg);
}
