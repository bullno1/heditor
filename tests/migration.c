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

	size_t mem_size = hgraph_registry_builder_init(NULL, 0, &registry_config);
	fixture.builder = arena_alloc(&fixture.base.arena, mem_size);

	hgraph_registry_builder_init(fixture.builder, mem_size, &registry_config);
}

TEST_TEARDOWN(migration) {
	fixture_cleanup(&fixture.base);
}

TEST(migration, missing_type) {
	hgraph_t* graph = fixture.base.graph;
	hgraph_registry_builder_t* builder = fixture.builder;
	plugin1_entry(
		hgraph_registry_builder_as_plugin_api(builder)
	);
	size_t reg_size = hgraph_registry_init(NULL, 0, builder);
	hgraph_registry_t* new_reg = arena_alloc(&fixture.base.arena, reg_size);
	hgraph_registry_init(new_reg, reg_size, builder);

	size_t migration_size = hgraph_migration_init(NULL, 0, fixture.base.registry, new_reg);
	hgraph_migration_t* migration = arena_alloc(&fixture.base.arena, migration_size);
	hgraph_migration_init(migration, migration_size, fixture.base.registry, new_reg);

	hgraph_config_t new_graph_config = {
		.registry = new_reg,
		.max_nodes = 64,
		.max_name_length = 64,
	};
	size_t new_graph_size = hgraph_init(NULL, 0, &new_graph_config);
	hgraph_t* new_graph = arena_alloc(&fixture.base.arena, new_graph_size);
	hgraph_init(new_graph, new_graph_size, &new_graph_config);

	hgraph_migration_execute(migration, fixture.base.graph, new_graph);

	hgraph_info_t old_info = hgraph_get_info(fixture.base.graph);
	ASSERT_EQ(old_info.num_nodes, 3);
	ASSERT_EQ(old_info.num_edges, 2);

	hgraph_info_t new_info = hgraph_get_info(new_graph);
	ASSERT_EQ(new_info.num_nodes, 2);
	ASSERT_EQ(new_info.num_edges, 0);

	ASSERT_EQ(
		hgraph_get_node_by_name(graph, HGRAPH_STR("start")),
		hgraph_get_node_by_name(new_graph, HGRAPH_STR("start"))
	);
	ASSERT_EQ(
		hgraph_get_node_by_name(graph, HGRAPH_STR("end")),
		hgraph_get_node_by_name(new_graph, HGRAPH_STR("end"))
	);
}

TEST(migration, identical) {
	// Delete and recreate nodes to shuffle the id space
	hgraph_t* graph = fixture.base.graph;
	hgraph_destroy_node(graph, hgraph_get_node_by_name(graph, HGRAPH_STR("mid")));
	hgraph_destroy_node(graph, hgraph_get_node_by_name(graph, HGRAPH_STR("start")));
	hgraph_destroy_node(graph, hgraph_get_node_by_name(graph, HGRAPH_STR("end")));
	ASSERT_EQ(hgraph_get_info(graph).num_nodes, 0);
	create_start_mid_end_graph(graph);

	// Migrate
	size_t migration_size = hgraph_migration_init(NULL, 0, fixture.base.registry, fixture.base.registry);
	hgraph_migration_t* migration = arena_alloc(&fixture.base.arena, migration_size);
	hgraph_migration_init(migration, migration_size, fixture.base.registry, fixture.base.registry);

	hgraph_config_t new_graph_config = {
		.registry = fixture.base.registry,
		.max_nodes = 64,
		.max_name_length = 64,
	};
	size_t new_graph_size = hgraph_init(NULL, 0, &new_graph_config);
	hgraph_t* new_graph = arena_alloc(&fixture.base.arena, new_graph_size);
	hgraph_init(new_graph, new_graph_size, &new_graph_config);

	// The id must be identical
	hgraph_migration_execute(migration, fixture.base.graph, new_graph);
	hgraph_info_t old_info = hgraph_get_info(new_graph);
	ASSERT_EQ(old_info.num_nodes, 3);
	ASSERT_EQ(old_info.num_edges, 2);

	ASSERT_EQ(
		hgraph_get_node_by_name(graph, HGRAPH_STR("start")),
		hgraph_get_node_by_name(new_graph, HGRAPH_STR("start"))
	);
	ASSERT_EQ(
		hgraph_get_node_by_name(graph, HGRAPH_STR("mid")),
		hgraph_get_node_by_name(new_graph, HGRAPH_STR("mid"))
	);
	ASSERT_EQ(
		hgraph_get_node_by_name(graph, HGRAPH_STR("end")),
		hgraph_get_node_by_name(new_graph, HGRAPH_STR("end"))
	);
}
