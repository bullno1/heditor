#include "plugin1.h"
#include "plugin2.h"
#include <munit.h>
#include <hgraph/runtime.h>

typedef struct {
	hgraph_registry_config_t config;
} fixture_t;

static MunitResult
reg(const MunitParameter params[], void* fixture) {
	(void)params;
	(void)fixture;

	hgraph_registry_config_t config = {
		.max_data_types = 32,
		.max_node_types = 32,
	};
	size_t mem_required = 0;
	hgraph_registry_builder_t* builder = hgraph_registry_builder_init(
		&config,
		NULL,
		&mem_required
	);
	munit_assert_null(builder);

	void* builder_mem = malloc(mem_required);
	builder = hgraph_registry_builder_init(
		&config,
		builder_mem,
		&mem_required
	);
	munit_assert_not_null(builder);

	hgraph_plugin_api_t* plugin_api = hgraph_registry_builder_as_plugin_api(builder);

	plugin1_entry(plugin_api);
	plugin2_entry(plugin_api);

	mem_required = 0;
	hgraph_registry_t* registry = hgraph_registry_init(builder, NULL, &mem_required);
	munit_assert_null(registry);

	void* registry_mem = malloc(mem_required);
	registry = hgraph_registry_init(builder, registry_mem, &mem_required);

	free(registry_mem);
	free(builder_mem);

	return MUNIT_OK;
}

MunitSuite test_registry = {
	.prefix = "/registry",
	.tests = (MunitTest[]){
		{
			.name = "/reg",
			.test = reg,
		},
		{ 0 },
	},
};
