#include "rktest.h"
#include "common.h"
#include "plugin1.h"
#include "plugin2.h"
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
		ASSERT_TRUE(node_type == &plugin1_start);
	} else if (node == i->end_id) {
		i->seen_end = true;
		ASSERT_TRUE(node_type == &plugin1_end);
	} else if (node == i->mid_id) {
		i->seen_mid = true;
		ASSERT_TRUE(node_type == &plugin2_mid);
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

static fixture_t fixture;

TEST_SETUP(graph) {
	fixture_init(&fixture);
}

TEST_TEARDOWN(graph) {
	fixture_cleanup(&fixture);
}

TEST(graph, connect) {
	hgraph_t* graph = fixture.graph;

	iterator_state i = { 0 };

	hgraph_iterate_nodes(graph, iterate_nodes, &i);
	hgraph_iterate_edges(graph, iterate_edges, &i);
	ASSERT_EQ(i.num_nodes, 0);
	ASSERT_EQ(i.num_edges, 0);

	i.start_id = hgraph_create_node(graph, &plugin1_start);
	hgraph_iterate_nodes(graph, iterate_nodes, &i);
	hgraph_iterate_edges(graph, iterate_edges, &i);
	ASSERT_EQ(i.num_nodes, 1);
	ASSERT_EQ(i.num_edges, 0);

	i.end_id = hgraph_create_node(graph, &plugin1_end);
	i.mid_id = hgraph_create_node(graph, &plugin2_mid);
	i.num_nodes = 0;
	hgraph_iterate_nodes(graph, iterate_nodes, &i);
	ASSERT_EQ(i.num_nodes, 3);
	ASSERT_TRUE(i.seen_start);
	ASSERT_TRUE(i.seen_mid);
	ASSERT_TRUE(i.seen_end);

	hgraph_index_t start_out = hgraph_get_pin_id(
		graph, i.start_id, &plugin1_start_out_f32
	);
	hgraph_index_t end_in = hgraph_get_pin_id(
		graph, i.end_id, &plugin1_end_in_i32
	);
	ASSERT_TRUE(HGRAPH_IS_VALID_INDEX(start_out));
	ASSERT_TRUE(HGRAPH_IS_VALID_INDEX(end_in));

	ASSERT_FALSE(
		HGRAPH_IS_VALID_INDEX(
			hgraph_connect(graph, start_out, end_in)
		)
	);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.start_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 0);

	hgraph_iterate_edges(graph, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 0);

	hgraph_index_t mid_in = hgraph_get_pin_id(
		graph, i.mid_id, &plugin2_mid_in_f32
	);
	hgraph_index_t mid_out = hgraph_get_pin_id(
		graph, i.mid_id, &plugin2_mid_out_i32
	);

	i.start_mid_id = hgraph_connect(graph, start_out, mid_in);
	// |start|->|mid|  |end|
	ASSERT_TRUE(HGRAPH_IS_VALID_INDEX(i.start_mid_id));
	i.num_edges = 0;
	hgraph_iterate_edges(graph, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 1);
	ASSERT_TRUE(i.seen_start_mid);
	ASSERT_FALSE(i.seen_mid_end);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.mid_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 0);

	i.mid_end_id = hgraph_connect(graph, mid_out, end_in);
	// |start|->|mid|->|end|
	ASSERT_TRUE(HGRAPH_IS_VALID_INDEX(i.mid_end_id));
	i.num_edges = 0;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_edges(graph, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 2);
	ASSERT_TRUE(i.seen_start_mid);
	ASSERT_TRUE(i.seen_mid_end);
	i.num_edges = 0;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_edges_from(graph, i.start_id, iterate_edges, &i);
	ASSERT_TRUE(i.seen_start_mid);
	ASSERT_FALSE(i.seen_mid_end);
	ASSERT_EQ(i.num_edges, 1);

	hgraph_disconnect(graph, i.start_mid_id);
	// |start|  |mid|->|end|
	i.num_edges = 0;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_edges(graph, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 1);
	ASSERT_FALSE(i.seen_start_mid);
	ASSERT_TRUE(i.seen_mid_end);

	i.start_mid_id = hgraph_connect(graph, start_out, mid_in);
	// |start|->|mid|->|end|
	i.num_edges = 0;
	i.seen_start_mid = false;
	i.seen_mid_end = false;
	hgraph_iterate_edges(graph, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 2);
	ASSERT_TRUE(i.seen_start_mid);
	ASSERT_TRUE(i.seen_mid_end);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.start_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 1);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.mid_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 1);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.end_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 0);

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
	ASSERT_EQ(i.num_nodes, 2);
	ASSERT_TRUE(i.seen_start);
	ASSERT_FALSE(i.seen_mid);
	ASSERT_TRUE(i.seen_end);
	ASSERT_EQ(i.num_edges, 0);

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
	ASSERT_EQ(i.num_nodes, 3);
	ASSERT_TRUE(i.seen_start);
	ASSERT_TRUE(i.seen_mid);
	ASSERT_TRUE(i.seen_end);
	ASSERT_EQ(i.num_edges, 0);

	hgraph_index_t mid2_id = hgraph_create_node(graph, &plugin2_mid);
	// |start|  |mid|  |end|
	//          |mid2|
	ASSERT_TRUE(
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
	ASSERT_TRUE(
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
	ASSERT_TRUE(
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
	ASSERT_EQ(i.num_nodes, 4);
	ASSERT_EQ(i.num_edges, 3);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.start_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 2);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.mid_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 1);
	i.num_edges = 0;
	hgraph_iterate_edges_from(graph, i.end_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 0);
	i.num_edges = 0;
	hgraph_iterate_edges_to(graph, i.start_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 0);
	i.num_edges = 0;
	hgraph_iterate_edges_to(graph, i.mid_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 1);
	i.num_edges = 0;
	hgraph_iterate_edges_to(graph, i.end_id, iterate_edges, &i);
	ASSERT_EQ(i.num_edges, 1);

	ASSERT_FALSE(
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
	ASSERT_TRUE(
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
}

TEST(graph, name) {
	hgraph_t* graph = fixture.graph;

	hgraph_index_t a = hgraph_create_node(
		graph, &plugin1_start
	);
	hgraph_index_t b = hgraph_create_node(
		graph, &plugin1_start
	);
	ASSERT_NE(a, b);

	hgraph_set_node_name(graph, a, HGRAPH_STR("a"));
	hgraph_set_node_name(graph, b, HGRAPH_STR("b"));

	hgraph_str_t name_a = hgraph_get_node_name(graph, a);
	hgraph_str_t name_b = hgraph_get_node_name(graph, b);
	ASSERT_EQ(memcmp(name_a.data, "a", name_a.length), 0);
	ASSERT_EQ(memcmp(name_b.data, "b", name_b.length), 0);

	ASSERT_EQ(hgraph_get_node_by_name(graph, HGRAPH_STR("a")), a);
	ASSERT_EQ(hgraph_get_node_by_name(graph, HGRAPH_STR("b")), b);
	ASSERT_FALSE(
		HGRAPH_IS_VALID_INDEX(
			hgraph_get_node_by_name(graph, HGRAPH_STR("c"))
		)
	);
}

TEST(graph, attribute) {
	hgraph_t* graph = fixture.graph;
	hgraph_index_t node = hgraph_create_node(graph, &plugin2_mid);
	const bool* round_up = hgraph_get_node_attribute(
		graph, node, &plugin2_mid_attr_round_up
	);
	ASSERT_TRUE(*round_up);

	hgraph_set_node_attribute(
		graph, node, &plugin2_mid_attr_round_up, &(bool){ false }
	);
	round_up = hgraph_get_node_attribute(
		graph, node, &plugin2_mid_attr_round_up
	);
	ASSERT_FALSE(*round_up);
}
