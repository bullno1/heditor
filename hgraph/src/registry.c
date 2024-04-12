#include "internal.h"
#include "mem_layout.h"
#include "hash.h"
#include <string.h>

const hgraph_node_type_t hgraph_dummy_node = { 0 };

HGRAPH_PRIVATE void
hgraph_registry_builder_plugin_register_node_type(
	const hgraph_plugin_api_t* api,
	const hgraph_node_type_t* node_type
) {
	hgraph_registry_builder_t* builder = HGRAPH_CONTAINER_OF(api, hgraph_registry_builder_t, plugin_api);
	hgraph_registry_builder_add(builder, node_type);
}

HGRAPH_PRIVATE hgraph_str_t
hgraph_alloc_string(char** pool, hgraph_str_t str) {
	char* storage = *pool;
	*pool = storage + str.length;

	if (str.length > 0) {
		memcpy(storage, str.data, str.length);
	}
	storage[str.length] = '\0';
	return (hgraph_str_t){ .data = storage, .length = str.length };
}

HGRAPH_PRIVATE hgraph_var_t*
hgraph_alloc_vars(hgraph_var_t** pool, hgraph_index_t num_vars) {
	hgraph_var_t* result = *pool;
	*pool = result + num_vars;
	return result;
}

HGRAPH_PRIVATE hgraph_output_buffer_info_t*
hgraph_alloc_output_buffers(hgraph_output_buffer_info_t** pool, hgraph_index_t num_buffers) {
	hgraph_output_buffer_info_t* result = *pool;
	*pool = result + num_buffers;
	return result;
}

HGRAPH_PRIVATE void
hgraph_registry_add_type(
	hgraph_registry_builder_t* builder,
	const hgraph_data_type_t* type
) {
	uint64_t hash = hash_murmur64((uintptr_t)type);
	hgraph_index_t i = (hgraph_index_t)hash;
	hgraph_index_t exp = builder->data_type_exp;
	const hgraph_data_type_t** data_types = builder->data_types;
	while (true) {
		i = hash_msi(hash, exp, i);
		if (data_types[i] == NULL) {
			data_types[i] = type;
			HGRAPH_ASSERT(
				builder->num_data_types < builder->config.max_data_types
			);
			++builder->num_data_types;
			return;
		} else if (data_types[i] == type) {
			return;
		}
	}
}

size_t
hgraph_registry_builder_init(
	hgraph_registry_builder_t* builder,
	const hgraph_registry_config_t* config
) {
	mem_layout_t layout = { 0 };
	mem_layout_reserve(
		&layout,
		sizeof(hgraph_registry_builder_t),
		_Alignof(hgraph_registry_builder_t)
	);
	ptrdiff_t node_types = mem_layout_reserve(
		&layout,
		// Reserve +1 space for dummy node type too
		sizeof(hgraph_node_type_t*) * (config->max_node_types + 1),
		_Alignof(hgraph_node_type_t*)
	);

	hgraph_index_t hash_data_type_exp = hash_exp(config->max_data_types);
	hgraph_index_t hash_data_type_size = hash_size(hash_data_type_exp);
	ptrdiff_t data_types = mem_layout_reserve(
		&layout,
		sizeof(hgraph_data_type_t*) * hash_data_type_size,
		_Alignof(hgraph_data_type_t*)
	);

	size_t required_size = mem_layout_size(&layout);
	if (builder == NULL) { return required_size; }

	memset(builder, 0, required_size);
	*builder = (hgraph_registry_builder_t){
		.config = *config,
		.plugin_api = {
			.register_node_type = &hgraph_registry_builder_plugin_register_node_type,
		},
		.data_type_exp = hash_data_type_exp,
		.node_types = mem_layout_locate(builder, node_types),
		.data_types = mem_layout_locate(builder, data_types),
	};

	hgraph_registry_builder_add(builder, &hgraph_dummy_node);

	return required_size;
}

void
hgraph_registry_builder_add(
	hgraph_registry_builder_t* builder,
	const hgraph_node_type_t* type
) {
	HGRAPH_ASSERT(
		// Take into account +1 slot for dummy node type
		builder->num_node_types < builder->config.max_node_types + 1
	);

	for (
		const hgraph_attribute_description_t** itr = type->attributes;
		itr != NULL && *itr != NULL;
		++itr
	) {
		const hgraph_attribute_description_t* attr = *itr;
		hgraph_registry_add_type(builder, attr->data_type);
	}

	for (
		const hgraph_pin_description_t** itr = type->input_pins;
		itr != NULL && *itr != NULL;
		++itr
	) {
		const hgraph_pin_description_t* pin = *itr;
		hgraph_registry_add_type(builder, pin->data_type);
	}

	for (
		const hgraph_pin_description_t** itr = type->output_pins;
		itr!= NULL && *itr != NULL;
		++itr
	) {
		const hgraph_pin_description_t* pin = *itr;
		hgraph_registry_add_type(builder, pin->data_type);
	}

	builder->node_types[builder->num_node_types++] = type;
}

hgraph_plugin_api_t*
hgraph_registry_builder_as_plugin_api(hgraph_registry_builder_t* builder) {
	return &builder->plugin_api;
}

size_t
hgraph_registry_init(
	hgraph_registry_t* registry,
	const hgraph_registry_builder_t* builder
) {
	size_t string_table_size = 0;
	hgraph_index_t num_vars = 0;
	hgraph_index_t num_outputs = 0;

	hgraph_index_t data_types_hash_size = hash_size(builder->data_type_exp);
	const hgraph_data_type_t** data_types = builder->data_types;
	for (hgraph_index_t i = 0; i < data_types_hash_size; ++i) {
		if (data_types[i] == NULL) { continue; }

		string_table_size += data_types[i]->name.length + 1;
	}

	hgraph_index_t num_node_types = builder->num_node_types;
	const hgraph_node_type_t** node_types = builder->node_types;
	for (hgraph_index_t i = 0; i < num_node_types; ++i) {
		const hgraph_node_type_t* node_type = node_types[i];
		string_table_size += node_type->name.length + 1;

		for (
			hgraph_index_t j = 0;
			node_type->attributes != NULL && node_type->attributes[j] != NULL;
			++j
		) {
			string_table_size += node_type->attributes[j]->name.length + 1;
			++num_vars;
		}

		for (
			hgraph_index_t j = 0;
			node_type->input_pins != NULL && node_type->input_pins[j] != NULL;
			++j
		) {
			string_table_size += node_type->input_pins[j]->name.length + 1;
			++num_vars;
		}

		for (
			hgraph_index_t j = 0;
			node_type->output_pins != NULL && node_type->output_pins[j] != NULL;
			++j
		) {
			string_table_size += node_type->output_pins[j]->name.length + 1;
			++num_vars;
			++num_outputs;
		}
	}

	mem_layout_t layout = { 0 };
	mem_layout_reserve(
		&layout,
		sizeof(hgraph_registry_t),
		_Alignof(hgraph_registry_t)
	);
	ptrdiff_t data_types_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_data_type_info_t) * builder->num_data_types,
		_Alignof(hgraph_data_type_info_t)
	);
	ptrdiff_t node_types_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_node_type_info_t) * builder->num_node_types,
		_Alignof(hgraph_node_type_info_t)
	);
	ptrdiff_t vars_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_var_t) * num_vars,
		_Alignof(hgraph_var_t)
	);
	ptrdiff_t output_buffers_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_output_buffer_info_t) * num_outputs,
		_Alignof(hgraph_output_buffer_info_t)
	);
	ptrdiff_t string_table_offset = mem_layout_reserve(
		&layout,
		string_table_size,
		_Alignof(char)
	);
	ptrdiff_t data_type_by_definition_offset = hgraph_ptr_table_reserve(
		&layout, builder->num_data_types
	);
	ptrdiff_t node_type_by_definition_offset = hgraph_ptr_table_reserve(
		&layout, builder->num_node_types
	);

	size_t required_size = mem_layout_size(&layout);
	if (registry == NULL) { return required_size; }

	*registry = (hgraph_registry_t){
		.num_data_types = builder->num_data_types,
		.num_node_types = builder->num_node_types,
		.data_types = mem_layout_locate(registry, data_types_offset),
		.node_types = mem_layout_locate(registry, node_types_offset),
	};
	hgraph_var_t* vars = mem_layout_locate(registry, vars_offset);
	hgraph_output_buffer_info_t* output_buffers = mem_layout_locate(registry, output_buffers_offset);
	char* str_table = mem_layout_locate(registry, string_table_offset);
	hgraph_ptr_table_init(
		&registry->data_type_by_definition,
		builder->num_data_types,
		mem_layout_locate(registry, data_type_by_definition_offset)
	);
	hgraph_ptr_table_init(
		&registry->node_type_by_definition,
		builder->num_node_types,
		mem_layout_locate(registry, node_type_by_definition_offset)
	);

	hgraph_index_t data_type_index = 0;
	for (hgraph_index_t i = 0; i < data_types_hash_size; ++i) {
		const hgraph_data_type_t* data_type_def = data_types[i];
		if (data_type_def == NULL) { continue; }

		hgraph_data_type_info_t* data_type_info = &registry->data_types[data_type_index++];
		*data_type_info = (hgraph_data_type_info_t){
			.name = hgraph_alloc_string(&str_table, data_type_def->name),
			.size = data_type_def->size,
			.definition = data_type_def,
		};
		hgraph_ptr_table_put(
			&registry->data_type_by_definition,
			data_type_def,
			data_type_info
		);
	}

	size_t max_node_size = 0;
	hgraph_index_t max_edges_per_node = 0;
	for (hgraph_index_t i = 0; i < num_node_types; ++i) {
		const hgraph_node_type_t* node_type_def = node_types[i];
		hgraph_node_type_info_t* node_type_info = &registry->node_types[i];
		*node_type_info = (hgraph_node_type_info_t){
			.name = hgraph_alloc_string(&str_table, node_type_def->name),
			.definition = node_type_def,
		};

		mem_layout_t node_layout = { 0 };
		mem_layout_reserve(
			&node_layout,
			sizeof(hgraph_node_t),
			_Alignof(hgraph_node_t)
		);

		hgraph_index_t num_attributes = 0;
		for (
			hgraph_index_t j = 0;
			node_type_def->attributes != NULL && node_type_def->attributes[j] != NULL;
			++j
		) {
			++num_attributes;
		}
		hgraph_var_t* attributes = hgraph_alloc_vars(&vars, num_attributes);
		for (hgraph_index_t j = 0; j < num_attributes; ++j) {
			const hgraph_attribute_description_t* attribute_def = node_type_def->attributes[j];
			hgraph_data_type_info_t* data_type_info = hgraph_ptr_table_lookup(
				&registry->data_type_by_definition,
				attribute_def->data_type
			);
			HGRAPH_ASSERT(data_type_info != NULL);
			attributes[j] = (hgraph_var_t){
				.type = data_type_info - registry->data_types,
				.name = hgraph_alloc_string(&str_table, attribute_def->name),
				// attributes are organized into a block
				.offset = mem_layout_reserve(
					&node_layout,
					attribute_def->data_type->size,
					attribute_def->data_type->alignment
				),
			};
		}
		node_type_info->num_attributes = num_attributes;
		node_type_info->attributes = attributes;

		hgraph_index_t num_input_pins = 0;
		for (
			hgraph_index_t j = 0;
			node_type_def->input_pins != NULL && node_type_def->input_pins[j] != NULL;
			++j
		) {
			++num_input_pins;
		}
		HGRAPH_ASSERT(num_input_pins <= HGRAPH_MAX_PINS);
		hgraph_bitset_t required_inputs;
		hgraph_bitset_init(&required_inputs);
		hgraph_var_t* input_pins = hgraph_alloc_vars(&vars, num_input_pins);
		for (hgraph_index_t j = 0; j < num_input_pins; ++j) {
			const hgraph_pin_description_t* pin_def = node_type_def->input_pins[j];
			hgraph_data_type_info_t* data_type_info = hgraph_ptr_table_lookup(
				&registry->data_type_by_definition,
				pin_def->data_type
			);
			HGRAPH_ASSERT(data_type_info != NULL);
			input_pins[j] = (hgraph_var_t){
				.type = data_type_info - registry->data_types,
				.name = hgraph_alloc_string(&str_table, pin_def->name),
				// Input pins are just a series of edge ids
				.offset = mem_layout_reserve(
					&node_layout,
					sizeof(hgraph_index_t),
					_Alignof(hgraph_index_t)
				),
			};

			if (pin_def->flow_type == HGRAPH_FLOW_ONCE) {
				hgraph_bitset_set(&required_inputs, j);
			}
		}
		node_type_info->num_input_pins = num_input_pins;
		node_type_info->input_pins = input_pins;
		node_type_info->required_inputs = required_inputs;
		max_edges_per_node = HGRAPH_MAX(max_edges_per_node, num_input_pins);

		mem_layout_t pipeline_data_layout = { 0 };
		mem_layout_reserve(
			&pipeline_data_layout,
			node_type_def->size,
			node_type_def->alignment
		);

		hgraph_index_t num_output_pins = 0;
		for (
			hgraph_index_t j = 0;
			node_type_def->output_pins != NULL && node_type_def->output_pins[j] != NULL;
			++j
		) {
			++num_output_pins;
		}
		HGRAPH_ASSERT(num_output_pins <= HGRAPH_MAX_PINS);
		hgraph_bitset_t required_outputs;
		hgraph_bitset_init(&required_outputs);
		hgraph_var_t* output_pins = hgraph_alloc_vars(&vars, num_output_pins);
		hgraph_output_buffer_info_t* node_output_buffers = hgraph_alloc_output_buffers(
			&output_buffers, num_output_pins
		);
		for (hgraph_index_t j = 0; j < num_output_pins; ++j) {
			const hgraph_pin_description_t* pin_def = node_type_def->output_pins[j];
			hgraph_data_type_info_t* data_type_info = hgraph_ptr_table_lookup(
				&registry->data_type_by_definition,
				pin_def->data_type
			);
			HGRAPH_ASSERT(data_type_info != NULL);
			output_pins[j] = (hgraph_var_t){
				.type = data_type_info - registry->data_types,
				.name = hgraph_alloc_string(&str_table, pin_def->name),
				// Ouput pins are linked lists
				.offset = mem_layout_reserve(
					&node_layout,
					sizeof(hgraph_edge_link_t),
					_Alignof(hgraph_edge_link_t)
				),
			};

			node_output_buffers[j] = (hgraph_output_buffer_info_t){
				.offset = mem_layout_reserve(
					&pipeline_data_layout,
					pin_def->data_type->size,
					pin_def->data_type->alignment
				),
			};

			if (pin_def->flow_type == HGRAPH_FLOW_ONCE) {
				hgraph_bitset_set(&required_outputs, j);
			}
		}
		node_type_info->num_output_pins = num_output_pins;
		node_type_info->output_pins = output_pins;
		node_type_info->required_outputs = required_outputs;
		node_type_info->output_buffers = node_output_buffers;

		node_type_info->size = mem_layout_size(&node_layout);
		node_type_info->pipeline_data_size = mem_layout_size(&pipeline_data_layout);
		max_node_size = HGRAPH_MAX(max_node_size, node_type_info->size);

		hgraph_ptr_table_put(
			&registry->node_type_by_definition,
			node_type_def,
			node_type_info
		);
	}

	registry->max_node_size = max_node_size;
	registry->max_edges_per_node = max_edges_per_node;

	return required_size;
}

void
hgraph_registry_iterate(
	const hgraph_registry_t* registry,
	hgraph_registry_iterator_t iterator,
	void* userdata
) {
	// Start from 1 to skip the dummy type
	for (hgraph_index_t i = 1; i < registry->num_node_types; ++i) {
		if (!iterator(registry->node_types[i].definition, userdata)) {
			break;
		}
	}
}
