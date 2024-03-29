#include "hgraph/common.h"
#include "plugin1.h"
#include "plugin2.h"
#include <munit.h>
#include <hgraph/runtime.h>

typedef struct {
	hgraph_index_t num_nodes;
	hgraph_index_t num_edges;

	hgraph_index_t start_id;
	hgraph_index_t mid_id;
	hgraph_index_t end_id;

	bool seen_start;
	bool seen_mid;
	bool seen_end;

	hgraph_index_t start_mid_id;
	hgraph_index_t mid_end_id;

	bool seen_start_mid;
	bool seen_mid_end;
} iterator_state;

typedef struct {
	hgraph_t* graph;
	hgraph_registry_t* registry;
} fixture_t;

static bool
iterate_nodes(
	hgraph_index_t node,
	const hgraph_node_type_t* node_type,
	void* userdata
) {
	iterator_state* i = userdata;
	++i->num_nodes;

	if (node == i->start_id) {
		i->seen_start = true;
		munit_assert_ptr(node_type, ==, &plugin1_start);
	} else if (node == i->end_id) {
		i->seen_end = true;
		munit_assert_ptr(node_type, ==, &plugin1_end);
	} else if (node == i->mid_id) {
		i->seen_mid = true;
		munit_assert_ptr(node_type, ==, &plugin2_mid);
	}

	return true;
}

static bool
iterate_edges(
	hgraph_index_t edge,
	hgraph_index_t from_pin,
	hgraph_index_t to_pin,
	void* userdata
) {
	(void)edge;
	(void)from_pin;
	(void)to_pin;

	iterator_state* i = userdata;
	++i->num_edges;

	if (edge == i->start_mid_id) {
		i->seen_start_mid = true;
	} else if (edge == i->mid_end_id) {
		i->seen_mid_end = true;
	}

	return true;
}

static void*
setup(const MunitParameter params[], void* user_data) {
	(void)params;
	(void)user_data;

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
	munit_assert_not_null(graph);

	fixture_t* fixture = malloc(sizeof(fixture_t));
	*fixture = (fixture_t){
		.graph = graph,
		.registry = registry,
	};

	return fixture;
}

static void
tear_down(void* fixture_p) {
	fixture_t* fixture = fixture_p;
	free(fixture->graph);
	free(fixture->registry);
	free(fixture);
}

static MunitResult
connect(const MunitParameter params[], void* fixture_p) {
	(void)params;

	fixture_t* fixture = fixture_p;
	hgraph_t* graph = fixture->graph;

	iterator_state i = { 0 };

	hgraph_iterate_nodes(graph, iterate_nodes, &i);
	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_nodes, ==, 0);
	munit_assert_int32(i.num_edges, ==, 0);

	i.start_id = hgraph_create_node(graph, &plugin1_start);
	hgraph_iterate_nodes(graph, iterate_nodes, &i);
	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_nodes, ==, 1);
	munit_assert_int32(i.num_edges, ==, 0);

	i.end_id = hgraph_create_node(graph, &plugin1_end);
	i.mid_id = hgraph_create_node(graph, &plugin2_mid);
	i.num_nodes = 0;
	hgraph_iterate_nodes(graph, iterate_nodes, &i);
	munit_assert_int32(i.num_nodes, ==, 3);
	munit_assert_true(i.seen_start);
	munit_assert_true(i.seen_mid);
	munit_assert_true(i.seen_end);

	hgraph_index_t start_out = hgraph_get_pin_id(
		graph, i.start_id, &plugin1_start_out_f32
	);
	hgraph_index_t end_in = hgraph_get_pin_id(
		graph, i.end_id, &plugin1_end_in_i32
	);
	munit_assert_true(HGRAPH_IS_VALID_INDEX(start_out));
	munit_assert_true(HGRAPH_IS_VALID_INDEX(end_in));

	munit_assert_false(
		HGRAPH_IS_VALID_INDEX(
			hgraph_connect(graph, start_out, end_in)
		)
	);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.start_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 0);

	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 0);

	hgraph_index_t mid_in = hgraph_get_pin_id(
		graph, i.mid_id, &plugin2_mid_in_f32
	);
	hgraph_index_t mid_out = hgraph_get_pin_id(
		graph, i.mid_id, &plugin2_mid_out_i32
	);

	i.start_mid_id = hgraph_connect(graph, start_out, mid_in);
	// |start|->|mid|  |end|
	munit_assert(HGRAPH_IS_VALID_INDEX(i.start_mid_id));
	i.num_edges = 0;
	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 1);
	munit_assert(i.seen_start_mid);
	munit_assert_false(i.seen_mid_end);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.mid_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 0);

	i.mid_end_id = hgraph_connect(graph, mid_out, end_in);
	// |start|->|mid|->|end|
	munit_assert(HGRAPH_IS_VALID_INDEX(i.mid_end_id));
	i.num_edges = 0;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 2);
	munit_assert(i.seen_start_mid);
	munit_assert(i.seen_mid_end);
	i.num_edges = 0;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_edges_from(graph, i.start_id, iterate_edges, &i);
	munit_assert(i.seen_start_mid);
	munit_assert(!i.seen_mid_end);
	munit_assert_int32(i.num_edges, ==, 1);

	hgraph_disconnect(graph, i.start_mid_id);
	// |start|  |mid|->|end|
	i.num_edges = 0;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 1);
	munit_assert_false(i.seen_start_mid);
	munit_assert(i.seen_mid_end);

	i.start_mid_id = hgraph_connect(graph, start_out, mid_in);
	// |start|->|mid|->|end|
	i.num_edges = 0;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 2);
	munit_assert(i.seen_start_mid);
	munit_assert(i.seen_mid_end);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.start_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 1);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.mid_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 1);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.end_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 0);

	hgraph_destroy_node(graph, i.mid_id);
	// |start|         |end|
	i.num_edges = 0;
	i.num_nodes = 0;
	i.seen_start = false;
	i.seen_mid = false;
	i.seen_end = false;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_nodes(graph, iterate_nodes, &i);
	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_nodes, ==, 2);
	munit_assert(i.seen_start);
	munit_assert(!i.seen_mid);
	munit_assert(i.seen_end);
	munit_assert_int32(i.num_edges, ==, 0);

	i.mid_id = hgraph_create_node(graph, &plugin2_mid);
	// |start|  |mid|  |end|
	i.num_edges = 0;
	i.num_nodes = 0;
	i.seen_start = false;
	i.seen_mid = false;
	i.seen_end = false;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_nodes(graph, iterate_nodes, &i);
	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_nodes, ==, 3);
	munit_assert(i.seen_start);
	munit_assert(i.seen_mid);
	munit_assert(i.seen_end);
	munit_assert_int32(i.num_edges, ==, 0);

	hgraph_index_t mid2_id = hgraph_create_node(graph, &plugin2_mid);
	// |start|  |mid|  |end|
	//          |mid2|
	munit_assert(
		HGRAPH_IS_VALID_INDEX(
			hgraph_connect(
				graph,
				hgraph_get_pin_id(graph, i.start_id, &plugin1_start_out_f32),
				hgraph_get_pin_id(graph, i.mid_id, &plugin2_mid_in_f32)
			)
		)
	);
	// |start|->|mid|  |end|
	//          |mid2|
	munit_assert(
		HGRAPH_IS_VALID_INDEX(
			hgraph_connect(
				graph,
				hgraph_get_pin_id(graph, i.start_id, &plugin1_start_out_f32),
				hgraph_get_pin_id(graph, mid2_id, &plugin2_mid_in_f32)
			)
		)
	);
	// |start|->|mid|  |end|
	//    └---->|mid2|
	munit_assert(
		HGRAPH_IS_VALID_INDEX(
			hgraph_connect(
				graph,
				hgraph_get_pin_id(graph, i.mid_id, &plugin2_mid_out_i32),
				hgraph_get_pin_id(graph, i.end_id, &plugin1_end_in_i32)
			)
		)
	);
	// |start|->|mid|->|end|
	//    └---->|mid2|
	i.num_edges = 0;
	i.num_nodes = 0;
	i.seen_start = false;
	i.seen_mid = false;
	i.seen_end = false;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_nodes(graph, iterate_nodes, &i);
	hgraph_iterate_edges(graph, iterate_edges, &i);
	munit_assert_int32(i.num_nodes, ==, 4);
	munit_assert_int32(i.num_edges, ==, 3);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.start_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 2);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.mid_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 1);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.end_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 0);
	i.num_edges = 0;
	hgraph_iterate_edges_to(graph, i.start_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 0);
	i.num_edges = 0;
	hgraph_iterate_edges_to(graph, i.mid_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 1);
	i.num_edges = 0;
	hgraph_iterate_edges_to(graph, i.end_id, iterate_edges, &i);
	munit_assert_int32(i.num_edges, ==, 1);

	munit_assert_false(
		HGRAPH_IS_VALID_INDEX(
			hgraph_connect(
				graph,
				hgraph_get_pin_id(graph, mid2_id, &plugin2_mid_out_i32),
				hgraph_get_pin_id(graph, i.end_id, &plugin1_end_in_i32)
			)
		)
	);
	hgraph_destroy_node(graph, i.mid_id);
	// |start|         |end|
	//    └---->|mid2|
	munit_assert(
		HGRAPH_IS_VALID_INDEX(
			hgraph_connect(
				graph,
				hgraph_get_pin_id(graph, mid2_id, &plugin2_mid_out_i32),
				hgraph_get_pin_id(graph, i.end_id, &plugin1_end_in_i32)
			)
		)
	);
	// |start|           ┌-->end|
	//    └---->|mid2|---┘

	return MUNIT_OK;
}

static MunitResult
name(const MunitParameter params[], void* fixture) {
	(void)params;

	hgraph_t* graph = ((fixture_t*)fixture)->graph;

	hgraph_index_t a = hgraph_create_node(
		graph, &plugin1_start
	);
	hgraph_index_t b = hgraph_create_node(
		graph, &plugin1_start
	);

	hgraph_set_node_name(graph, a, HGRAPH_STR("a"));
	hgraph_set_node_name(graph, b, HGRAPH_STR("b"));

	hgraph_str_t name_a = hgraph_get_node_name(graph, a);
	hgraph_str_t name_b = hgraph_get_node_name(graph, b);
	munit_assert_memory_equal(name_a.length, name_a.data, "a");
	munit_assert_memory_equal(name_b.length, name_b.data, "b");

	munit_assert_int32(
		hgraph_get_node_by_name(graph, HGRAPH_STR("a")),
		==,
		a
	);
	munit_assert_int32(
		hgraph_get_node_by_name(graph, HGRAPH_STR("b")),
		==,
		b
	);
	munit_assert_false(
		HGRAPH_IS_VALID_INDEX(
			hgraph_get_node_by_name(graph, HGRAPH_STR("c"))
		)
	);

	return MUNIT_OK;
}

MunitSuite test_graph = {
	.prefix = "/graph",
	.tests = (MunitTest[]){
		{
			.name = "/connect",
			.test = connect,
			.setup = setup,
			.tear_down = tear_down,
		},
		{
			.name = "/name",
			.test = name,
			.setup = setup,
			.tear_down = tear_down,
		},
		{ 0 },
	},
};
