#ifndef HEDITOR_IO_H
#define HEDITOR_IO_H

#include <hgraph/common.h>
#include <stdio.h>

struct hgraph_s;
struct neEditorContext;

hgraph_io_status_t
save_graph(const struct hgraph_s* graph, FILE* file);

hgraph_io_status_t
load_graph(struct hgraph_s* graph, FILE* file);

#endif
