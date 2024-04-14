#include "node_type_menu.h"
#include "allocator/arena.h"
#include <hgraph/runtime.h>
#include <string.h>

typedef struct {
	hed_arena_t* arena;
	node_type_menu_entry_t root;
	int num_nodes;
	size_t string_size;
} build_menu_tree_ctx_t;

typedef struct {
	node_type_menu_entry_t* node_pool;
	char* str_pool;
} flatten_menu_tree_ctx_t;

static inline bool
build_menu_tree(
	const hgraph_node_type_t* node_type,
	void* userdata
) {
	build_menu_tree_ctx_t* ctx = userdata;

	const char* group_name = node_type->group.data;
	const char* pos;
	node_type_menu_entry_t* node = &ctx->root;
	do {
		pos = strchr(group_name, '/');
		if (pos == NULL) { pos = group_name + strlen(group_name); }

		hed_str_t label = {
			.data = group_name,
			.length = pos - group_name
		};
		ctx->string_size += label.length + 1;

		// Attempt to find a child node with the same label
		node_type_menu_entry_t* child = NULL;
		for (
			node_type_menu_entry_t* itr = node->children;
			itr != NULL;
			itr = itr->next_sibling
		) {
			if (hed_str_equal(itr->label, label)) {
				child = itr;
				break;
			}
		}

		// Create a new one if it does not exist
		if (child == NULL) {
			child = HED_ARENA_ALLOC_TYPE(
				ctx->arena, node_type_menu_entry_t
			);
			*child = (node_type_menu_entry_t){
				.label = label,
			};
			child->next_sibling = node->children;
			node->children = child;
			++ctx->num_nodes;
		}

		// Descend
		node = child;
		group_name = pos + 1;
	} while (pos < node_type->group.data + node_type->group.length);

	node_type_menu_entry_t* entry = HED_ARENA_ALLOC_TYPE(
		ctx->arena, node_type_menu_entry_t
	);
	*entry = (node_type_menu_entry_t){
		.node_type = node_type,
	};
	entry->next_sibling = node->children;
	node->children = entry;
	++ctx->num_nodes;

	return true;
}

static inline void
flatten_menu_tree(
	flatten_menu_tree_ctx_t* ctx,
	const node_type_menu_entry_t* from_node,
	node_type_menu_entry_t* to_node
) {
	hed_str_t label = from_node->label;
	char* str_copy = ctx->str_pool;
	ctx->str_pool += label.length + 1;
	if (label.data != NULL) {
		memcpy(str_copy, label.data, label.length);
	}
	str_copy[label.length] = '\0';
	to_node->node_type = from_node->node_type;
	to_node->label = (hed_str_t){
		.data = str_copy,
		.length = label.length
	};
	to_node->children = NULL;
	to_node->next_sibling = NULL;

	node_type_menu_entry_t* last_node = NULL;
	for (
		node_type_menu_entry_t* itr = from_node->children;
		itr != NULL;
		itr = itr->next_sibling
	) {
		node_type_menu_entry_t* node_copy = ctx->node_pool++;
		flatten_menu_tree(ctx, itr, node_copy);
		if (last_node == NULL) {
			to_node->children = node_copy;
		} else {
			last_node->next_sibling = node_copy;
		}

		last_node = node_copy;
	}
}

size_t
node_type_menu_init(
	node_type_menu_entry_t* menu,
	struct hgraph_registry_s* registry,
	struct hed_arena_s* arena
) {
	size_t required_size = 0;
	HED_WITH_ARENA(arena) {
		build_menu_tree_ctx_t build_ctx = {
			.arena = arena,
		};
		hgraph_registry_iterate(
			registry,
			build_menu_tree,
			&build_ctx
		);

		required_size =
			sizeof(node_type_menu_entry_t) * (build_ctx.num_nodes + 1)  // Include root
			+ build_ctx.string_size;
		if (menu != NULL) {
			flatten_menu_tree_ctx_t flatten_ctx = {
				.node_pool = menu,
				.str_pool = (char*)menu + sizeof(node_type_menu_entry_t) * (build_ctx.num_nodes + 1),
			};
			node_type_menu_entry_t* root = flatten_ctx.node_pool++;
			*root = (node_type_menu_entry_t){ 0 };
			flatten_menu_tree(&flatten_ctx, &build_ctx.root, root);
		}
	}

	return required_size;
}
