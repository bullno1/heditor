#include "ptr_table.h"
#include "mem_layout.h"
#include "hash.h"
#include <string.h>
#include <stdbool.h>

ptrdiff_t
hgraph_ptr_table_reserve(mem_layout_t* layout, hgraph_index_t num_entries) {
	hgraph_index_t exp = hash_exp(num_entries);
	hgraph_index_t size = hash_size(exp);
	return mem_layout_reserve(
		layout,
		sizeof(hgraph_ptr_pair_t) * size,
		_Alignof(hgraph_ptr_pair_t)
	);
}

void
hgraph_ptr_table_init(
	hgraph_ptr_table_t* table,
	hgraph_index_t num_entries,
	void* entries
) {
	hgraph_index_t exp = hash_exp(num_entries);
	hgraph_index_t size = hash_size(exp);

	table->exp = exp;
	table->entries = entries;
	memset(table->entries, 0, sizeof(hgraph_ptr_pair_t) * size);
}

void
hgraph_ptr_table_put(
	hgraph_ptr_table_t* table,
	const void* key,
	void* value
) {
	uint64_t hash = hash_murmur64((uintptr_t)key);
	hgraph_index_t i = (hgraph_index_t)hash;
	hgraph_index_t exp = table->exp;
	hgraph_ptr_pair_t* entries = table->entries;
	while (true) {
		i = hash_msi(hash, exp, i);
		hgraph_ptr_pair_t* entry = &entries[i];
		if (entry->key == NULL || entry->key == key) {
			entry->value = value;
			return;
		}
	}
}

void*
hgraph_ptr_table_lookup(
	hgraph_ptr_table_t* table,
	const void* key
) {
	uint64_t hash = hash_murmur64((uintptr_t)key);
	hgraph_index_t i = (hgraph_index_t)hash;
	hgraph_index_t exp = table->exp;
	hgraph_ptr_pair_t* entries = table->entries;
	while (true) {
		i = hash_msi(hash, exp, i);
		hgraph_ptr_pair_t* entry = &entries[i];
		if (entry->key == NULL) {
			return NULL;
		} else if (entry->key == key) {
			return entry->value;
		}
	}
}
