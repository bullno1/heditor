#ifndef HGRAPH_CORE_REG_H
#define HGRAPH_CORE_REG_H

#include <autolist.h>

#define HGRAPH_CORE_NODE(NAME) \
	AUTOLIST_ENTRY(hgraph_core_nodes, const hgraph_node_type_t, NAME)

#endif
