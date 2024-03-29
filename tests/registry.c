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

	void* mem = malloc(mem_required);
	builder = hgraph_registry_builder_init(
		&config,
		mem,
		&mem_required
	);
	munit_assert_not_null(builder);

	hgraph_plugin_api_t* plugin_api = hgraph_registry_builder_as_plugin_api(builder);

	plugin1_entry(plugin_api);
	plugin2_entry(plugin_api);

	free(mem);
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
