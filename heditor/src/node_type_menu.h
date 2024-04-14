#ifndef HED_NODE_TYPE_MENU_H
#define HED_NODE_TYPE_MENU_H

#include <stddef.h>
#include "hstring.h"

typedef struct node_type_menu_entry_s {
	hed_str_t label;

	const struct hgraph_node_type_s* node_type;
	struct node_type_menu_entry_s* next_sibling;
	struct node_type_menu_entry_s* children;
} node_type_menu_entry_t;

struct hgraph_registry_s;
struct hed_arena_s;

size_t
node_type_menu_init(
	node_type_menu_entry_t* menu,
	struct hgraph_registry_s* registry,
	struct hed_arena_s* arena
);

#endif
