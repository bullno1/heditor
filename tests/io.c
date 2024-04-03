#include "hgraph/common.h"
#include "hgraph/runtime.h"
#include "rktest.h"
#include "common.h"
#include "../hgraph/src/internal.h"

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

	size_t mem_size = hgraph_init(NULL, &graph_config);
	hgraph_t* graph = arena_alloc(&fixture.base.arena, mem_size);
	hgraph_init(graph, &graph_config);
	ASSERT_EQ(hgraph_read_graph(&header, graph, &mem_io->in), HGRAPH_IO_OK);
	ASSERT_TRUE(mem_io->pos == mem_io->end);

	hgraph_info_t graph_info = hgraph_get_info(graph);
	ASSERT_EQ(graph_info.num_nodes, 3);
	ASSERT_EQ(graph_info.num_edges, 2);
}