#include "io.h"
#include "hgraph/common.h"
#include "hgraph/io.h"
#include "hgraph/runtime.h"
#include "utils.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cnode-editor.h"

typedef struct {
	hgraph_in_t in;
	hgraph_out_t out;
	FILE* handle;
} std_file_t;

typedef struct {
	const hgraph_t* graph;
	neEditorContext* editor;
	std_file_t file;
	hgraph_io_status_t status;
} graph_io_ctx_t;

static size_t
write_file(
	struct hgraph_out_s* output,
	const void* buffer,
	size_t size
) {
	std_file_t* std_file = HED_CONTAINER_OF(output, std_file_t, out);
	return fwrite(buffer, 1, size, std_file->handle);
}

static size_t
read_file(
	struct hgraph_in_s* input,
	void* buffer,
	size_t size
) {
	std_file_t* std_file = HED_CONTAINER_OF(input, std_file_t, in);
	return fread(buffer, 1, size, std_file->handle);
}

static hgraph_io_status_t
write_imvec2(ImVec2 vec2, hgraph_out_t* out) {
	HGRAPH_CHECK_IO(hgraph_io_write_f32(vec2.x, out));
	HGRAPH_CHECK_IO(hgraph_io_write_f32(vec2.y, out));
	return HGRAPH_IO_OK;
}

static hgraph_io_status_t
read_imvec2(ImVec2* vec2, hgraph_in_t* in) {
	HGRAPH_CHECK_IO(hgraph_io_read_f32(&vec2->x, in));
	HGRAPH_CHECK_IO(hgraph_io_read_f32(&vec2->y, in));
	return HGRAPH_IO_OK;
}

static bool
save_node_position(
	hgraph_index_t node_id,
	const hgraph_node_type_t* node_type,
	void* userdata
) {
	(void)node_type;
	graph_io_ctx_t* ctx = userdata;

	ImVec2 pos;
	neGetNodePosition(node_id, &pos);
	ctx->status = write_imvec2(pos, &ctx->file.out);

	return ctx->status == HGRAPH_IO_OK;
}

static bool
load_node_position(
	hgraph_index_t node_id,
	const hgraph_node_type_t* node_type,
	void* userdata
) {
	(void)node_type;
	graph_io_ctx_t* ctx = userdata;

	ImVec2 pos = { 0 };
	ctx->status = read_imvec2(&pos, &ctx->file.in);
	neSetNodePosition(node_id, pos);

	return ctx->status == HGRAPH_IO_OK;
}

hgraph_io_status_t
save_graph(
	const struct hgraph_s* graph,
	FILE* file
) {
	std_file_t std_file = {
		.out.write = write_file,
		.handle = file,
	};
	hgraph_out_t* out = &std_file.out;

	HGRAPH_CHECK_IO(hgraph_write_header(out));
	HGRAPH_CHECK_IO(hgraph_write_graph(graph, out));

	// Save position info
	graph_io_ctx_t io_ctx = {
		.graph = graph,
		.file = std_file,
		.status = HGRAPH_IO_OK,
	};
	hgraph_iterate_nodes(graph, save_node_position, &io_ctx);
	HGRAPH_CHECK_IO(io_ctx.status);

	return HGRAPH_IO_OK;
}

hgraph_io_status_t
load_graph(
	struct hgraph_s* graph,
	FILE* file
) {
	std_file_t std_file = {
		.in.read = read_file,
		.handle = file,
	};
	hgraph_in_t* in = &std_file.in;

	hgraph_header_t header;
	HGRAPH_CHECK_IO(hgraph_read_header(&header, in));
	hgraph_config_t config;
	HGRAPH_CHECK_IO(hgraph_read_graph_config(&header, &config, in));
	HGRAPH_CHECK_IO(hgraph_read_graph(&header, graph, in));

	// Load position info
	graph_io_ctx_t io_ctx = {
		.graph = graph,
		.file = std_file,
		.status = HGRAPH_IO_OK,
	};
	hgraph_iterate_nodes(graph, load_node_position, &io_ctx);
	HGRAPH_CHECK_IO(io_ctx.status);

	return HGRAPH_IO_OK;
}
