#include "hgraph/runtime.h"
#include "rktest.h"
#include "common.h"
#include "../hgraph/src/internal.h"
#include <hgraph/io.h>
#include <stdint.h>

typedef struct {
	hgraph_in_t in;
	hgraph_out_t out;
	char* pos;
	char* start;
	char* end;
} memory_io;

static size_t
memory_io_read(hgraph_in_t* in, void* buffer, size_t size) {
	memory_io* mem_io = HGRAPH_CONTAINER_OF(in, memory_io, in);
	size_t read_size = HGRAPH_MIN((size_t)(mem_io->end - mem_io->pos), size);
	memcpy(buffer, mem_io->pos, read_size);
	mem_io->pos += size;
	return size;
}

static size_t
memory_io_write(hgraph_out_t* out, const void* buffer, size_t size) {
	memory_io* mem_io = HGRAPH_CONTAINER_OF(out, memory_io, out);
	if (mem_io->pos + size > mem_io->end) { return 0; }
	memcpy(mem_io->pos, buffer, size);
	mem_io->pos += size;
	return size;
}

static hgraph_io_status_t
write_slip(const void* value, hgraph_out_t* out) {
	(void)value;
	for (uint8_t i = 0; i < UINT8_MAX; ++i) {
		HGRAPH_CHECK_IO(hgraph_io_write(out, &i, sizeof(i)));
	}

	return HGRAPH_IO_OK;
}

static hgraph_io_status_t
read_slip(void* value, hgraph_in_t* in) {
	(void)value;

	for (uint8_t i = 0; i < UINT8_MAX; ++i) {
		uint8_t tmp;
		HGRAPH_CHECK_IO(hgraph_io_read(in, &tmp, sizeof(tmp)));
		ASSERT_EQ(i, tmp);
	}

	return HGRAPH_IO_OK;
}

static struct {
	fixture_t base;
	memory_io mem_io;
} fixture;

TEST_SETUP(io) {
	fixture_init(&fixture.base);
	create_start_mid_end_graph(fixture.base.graph);

	size_t mem_io_size = 1024 * 1024;
	void* buf = arena_alloc(&fixture.base.arena, mem_io_size);
	fixture.mem_io = (memory_io){
		.in = { .read = memory_io_read },
		.out = { .write = memory_io_write },
		.pos = buf,
		.start = buf,
		.end = (char*)buf + mem_io_size,
	};
}

TEST_TEARDOWN(io) {
	fixture_cleanup(&fixture.base);
}

TEST(io, read_write_same) {
	memory_io* mem_io = &fixture.mem_io;

	ASSERT_EQ(hgraph_write_header(&mem_io->out), HGRAPH_IO_OK);
	ASSERT_EQ(hgraph_write_graph(fixture.base.graph, &mem_io->out), HGRAPH_IO_OK);
	mem_io->end = mem_io->pos;
	mem_io->pos = mem_io->start;

	hgraph_header_t header;
	ASSERT_EQ(hgraph_read_header(&header, &mem_io->in), HGRAPH_IO_OK);

	hgraph_config_t graph_config;
	ASSERT_EQ(hgraph_read_graph_config(&header, &graph_config, &mem_io->in), HGRAPH_IO_OK);
	graph_config.registry = fixture.base.registry;

	size_t mem_size = hgraph_init(NULL, 0, &graph_config);
	hgraph_t* graph = arena_alloc(&fixture.base.arena, mem_size);
	hgraph_init(graph, mem_size, &graph_config);
	ASSERT_EQ(hgraph_read_graph(&header, graph, &mem_io->in), HGRAPH_IO_OK);
	ASSERT_TRUE(mem_io->pos == mem_io->end);

	hgraph_info_t graph_info = hgraph_get_info(graph);
	ASSERT_EQ(graph_info.num_nodes, 3);
	ASSERT_EQ(graph_info.num_edges, 2);
}

TEST(io, slip) {
	// A data type that exists to test the writing of every single byte
	hgraph_data_type_t slip_data = {
		.serialize = write_slip,
		.deserialize = read_slip,
	};

	hgraph_attribute_description_t slip_attr = {
		.name = HGRAPH_STR("slip"),
		.data_type = &slip_data,
	};

	hgraph_node_type_t node_type = {
		.name = HGRAPH_STR("slip_test"),
		.attributes = HGRAPH_NODE_ATTRIBUTES(&slip_attr),
	};

	hgraph_registry_config_t registry_config = {
		.max_data_types = 1,
		.max_node_types = 1,
	};
	size_t mem_size = hgraph_registry_builder_init(NULL, 0, &registry_config);
	hgraph_registry_builder_t* builder = arena_alloc(&fixture.base.arena, mem_size);
	hgraph_registry_builder_init(builder, mem_size, &registry_config);

	hgraph_registry_builder_add(builder, &node_type);
	mem_size = hgraph_registry_init(NULL, 0, builder);
	hgraph_registry_t* registry = arena_alloc(&fixture.base.arena, mem_size);
	hgraph_registry_init(registry, mem_size, builder);

	hgraph_config_t graph_config = {
		.max_name_length = 1,
		.max_nodes = 1,
		.registry = registry,
	};
	mem_size = hgraph_init(NULL, 0, &graph_config);
	hgraph_t* graph = arena_alloc(&fixture.base.arena, mem_size);
	hgraph_init(graph, mem_size, &graph_config);
	hgraph_create_node(graph, &node_type);

	memory_io* mem_io = &fixture.mem_io;
	ASSERT_EQ(hgraph_write_header(&mem_io->out), HGRAPH_IO_OK);
	ASSERT_EQ(hgraph_write_graph(graph, &mem_io->out), HGRAPH_IO_OK);
	mem_io->end = mem_io->pos;
	mem_io->pos = mem_io->start;

	hgraph_header_t header;
	ASSERT_EQ(hgraph_read_header(&header, &mem_io->in), HGRAPH_IO_OK);

	ASSERT_EQ(hgraph_read_graph_config(&header, &graph_config, &mem_io->in), HGRAPH_IO_OK);
	graph_config.registry = registry;

	hgraph_init(graph, mem_size, &graph_config);
	ASSERT_EQ(hgraph_read_graph(&header, graph, &mem_io->in), HGRAPH_IO_OK);
	ASSERT_TRUE(mem_io->pos == mem_io->end);

	hgraph_info_t graph_info = hgraph_get_info(graph);
	ASSERT_EQ(graph_info.num_nodes, 1);
}
