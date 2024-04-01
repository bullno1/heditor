#include "internal.h"
#include "graph.h"

HGRAPH_PRIVATE hgraph_var_migration_plan_t*
hgraph_alloc_var_plans(
	hgraph_var_migration_plan_t** pool,
	hgraph_index_t num_plans
) {
	hgraph_var_migration_plan_t* result = *pool;
	*pool = result + num_plans;
	return result;
}

HGRAPH_PRIVATE hgraph_var_migration_plan_t*
hgraph_create_var_plans(
	hgraph_var_migration_plan_t** pool,
	const hgraph_data_type_info_t* from_data_types,
	const hgraph_data_type_info_t* to_data_types,
	const hgraph_var_t* from_vars, hgraph_index_t num_from_vars,
	const hgraph_var_t* to_vars, hgraph_index_t num_to_vars
) {
	hgraph_var_migration_plan_t* plans = hgraph_alloc_var_plans(
		pool, num_from_vars
	);

	for (hgraph_index_t i = 0; i < num_from_vars; ++i) {
		const hgraph_var_t* from_var = &from_vars[i];

		hgraph_index_t new_index = HGRAPH_INVALID_INDEX;
		for (hgraph_index_t j = 0; j < num_to_vars; ++i) {
			const hgraph_var_t* to_var = &to_vars[j];

			if (hgraph_str_equal(from_var->name, to_var->name)) {
				const hgraph_data_type_info_t* from_type = &from_data_types[from_var->type];
				const hgraph_data_type_info_t* to_type = &to_data_types[to_var->type];

				if (
					hgraph_str_equal(from_type->name, to_type->name)
					&& (from_type->size == to_type->size)
				) {
					new_index = j;
					break;
				}
			}
		}

		plans[i].new_index = new_index;
	}

	return plans;
}

size_t
hgraph_migration_init(
	hgraph_migration_t* migration,
	const hgraph_registry_t* from_registry,
	const hgraph_registry_t* to_registry
) {
	mem_layout_t layout = { 0 };
	mem_layout_reserve(
		&layout,
		sizeof(hgraph_migration_t),
		_Alignof(hgraph_migration_t)
	);
	ptrdiff_t node_plans_offset = mem_layout_reserve(
		&layout,
		// Exclude dummy
		sizeof(hgraph_node_migration_plan_t) * (from_registry->num_node_types - 1),
		_Alignof(hgraph_node_migration_plan_t)
	);

	hgraph_index_t num_var_plans = 0;
	for (hgraph_index_t i = 1; i < from_registry->num_node_types; ++i) {
		const hgraph_node_type_info_t* type_info = &from_registry->node_types[i];
		num_var_plans += type_info->num_attributes;
		num_var_plans += type_info->num_input_pins;
		num_var_plans += type_info->num_output_pins;
	}
	ptrdiff_t var_plans_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_var_migration_plan_t) * num_var_plans,
		_Alignof(hgraph_var_migration_plan_t)
	);

	size_t required_size = mem_layout_size(&layout);
	if (migration == NULL) { return required_size; }

	*migration = (hgraph_migration_t){
		.node_plans = mem_layout_locate(migration, node_plans_offset),
	};
	hgraph_var_migration_plan_t* var_plans = mem_layout_locate(
		migration, var_plans_offset
	);

	for (hgraph_index_t i = 1; i < from_registry->num_node_types; ++i) {
		const hgraph_node_type_info_t* from_type_info = &from_registry->node_types[i];

		// Find the new type, default to dummy
		const hgraph_node_type_info_t* to_type_info = &to_registry->node_types[0];
		for (hgraph_index_t j = 1; j < to_registry->num_node_types; ++j) {
			const hgraph_node_type_info_t* itr = &to_registry->node_types[i];

			if (hgraph_str_equal(from_type_info->name, itr->name)) {
				to_type_info = itr;
				break;
			}
		}

		const hgraph_data_type_info_t* from_data_types = from_registry->data_types;
		const hgraph_data_type_info_t* to_data_types = to_registry->data_types;
		hgraph_var_migration_plan_t* attribute_plans = hgraph_create_var_plans(
			&var_plans,
			from_data_types, to_data_types,
			from_type_info->attributes, from_type_info->num_attributes,
			to_type_info->attributes, to_type_info->num_attributes
		);
		hgraph_var_migration_plan_t* input_plans = hgraph_create_var_plans(
			&var_plans,
			from_data_types, to_data_types,
			from_type_info->input_pins, from_type_info->num_input_pins,
			to_type_info->input_pins, to_type_info->num_input_pins
		);
		hgraph_var_migration_plan_t* output_plans = hgraph_create_var_plans(
			&var_plans,
			from_data_types, to_data_types,
			from_type_info->output_pins, from_type_info->num_output_pins,
			to_type_info->output_pins, to_type_info->num_output_pins
		);

		migration->node_plans[i - 1] = (hgraph_node_migration_plan_t){
			.new_type = to_type_info,
			.attribute_plans = attribute_plans,
			.output_plans = output_plans,
			.input_plans = input_plans,
		};
	}

	return required_size;
}

void
hgraph_migration_execute(
	const hgraph_migration_t* migration,
	const hgraph_t* from_graph,
	hgraph_t* to_graph
) {
	// Migrate nodes
	for (hgraph_index_t i = 0; i < from_graph->node_slot_map.num_items; ++i) {
		const hgraph_node_t* from_node = hgraph_get_node_by_slot(from_graph, i);

		const hgraph_node_type_info_t* from_type_info = hgraph_get_node_type_internal(
			from_graph, from_node
		);

		const hgraph_node_migration_plan_t* node_plan = &migration->node_plans[from_node->type - 1];

		const hgraph_node_type_info_t* to_type_info = node_plan->new_type;
		hgraph_index_t to_node_id = hgraph_create_node(to_graph, to_type_info->definition);

		hgraph_set_node_name(
			to_graph,
			to_node_id,
			hgraph_get_node_name_internal(from_graph, from_node)
		);

		hgraph_node_t* to_node = hgraph_find_node_by_id(to_graph, to_node_id);
		HGRAPH_ASSERT(to_node != NULL);

		// Migrate attributes
		for (hgraph_index_t j = 0; j < from_type_info->num_attributes; ++j) {
			const hgraph_var_t* from_var = &from_type_info->attributes[j];

			hgraph_index_t to_index = node_plan->attribute_plans[j].new_index;
			if (!HGRAPH_IS_VALID_INDEX(to_index)) { continue; }

			const hgraph_var_t* to_var = &to_type_info->attributes[to_index];

			const char* from_storage = (char*)from_node + from_var->offset;
			char* to_storage = (char*)to_node + to_var->offset;
			const hgraph_data_type_info_t* to_type = &to_graph->registry->data_types[to_var->type];
			memcpy(to_storage, from_storage, to_type->size);
		}
	}

	// Migrate edges
	for (hgraph_index_t i = 0; i < from_graph->edge_slot_map.num_items; ++i) {
		const hgraph_edge_t* from_edge = &from_graph->edges[i];

		bool is_output;
		hgraph_index_t from_node_id, from_pin_index;
		hgraph_decode_pin_id(from_edge->from_pin, &from_node_id, &from_pin_index, &is_output);
		HGRAPH_ASSERT(is_output);

		hgraph_index_t to_node_id, to_pin_index;
		hgraph_decode_pin_id(from_edge->to_pin, &to_node_id, &to_pin_index, &is_output);
		HGRAPH_ASSERT(!is_output);

		hgraph_node_t* from_node = hgraph_find_node_by_id(from_graph, from_node_id);
		hgraph_node_t* to_node = hgraph_find_node_by_id(from_graph, to_node_id);
		HGRAPH_ASSERT(from_node != NULL);
		HGRAPH_ASSERT(to_node != NULL);

		const hgraph_node_migration_plan_t* from_node_plan = &migration->node_plans[from_node->type - 1];
		const hgraph_node_migration_plan_t* to_node_plan = &migration->node_plans[to_node->type - 1];
		hgraph_index_t new_from_pin_index = from_node_plan->output_plans[from_pin_index].new_index;
		hgraph_index_t new_to_pin_index = to_node_plan->input_plans[to_pin_index].new_index;
		if (!(HGRAPH_IS_VALID_INDEX(new_from_pin_index) && HGRAPH_IS_VALID_INDEX(new_to_pin_index))) {
			continue;
		}

		hgraph_index_t from_node_slot = ((char*)from_node - from_graph->nodes) / from_graph->node_size;
		hgraph_index_t to_node_slot = ((char*)to_node - from_graph->nodes) / from_graph->node_size;
		HGRAPH_ASSERT(from_node_slot < to_graph->node_slot_map.num_items);
		HGRAPH_ASSERT(to_node_slot < to_graph->node_slot_map.num_items);

		hgraph_index_t new_from_node_id = hgraph_slot_map_id_for_slot(
			&to_graph->node_slot_map, from_node_slot
		);
		hgraph_index_t new_to_node_id = hgraph_slot_map_id_for_slot(
			&to_graph->node_slot_map, to_node_slot
		);
		HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(new_from_node_id));
		HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(new_to_node_id));

		hgraph_index_t new_from_pin = hgraph_encode_pin_id(
			new_from_node_id,
			new_from_pin_index,
			true
		);
		hgraph_index_t new_to_pin = hgraph_encode_pin_id(
			new_to_node_id,
			new_to_pin_index,
			false
		);
		hgraph_connect(to_graph, new_from_pin, new_to_pin);
	}

	// Delete dummy nodes
	for (hgraph_index_t i = 0; i < to_graph->node_slot_map.num_items;) {
		const hgraph_node_t* node = hgraph_get_node_by_slot(to_graph, i);
		if (node->type == 0) {
			hgraph_index_t id = hgraph_slot_map_id_for_slot(
				&to_graph->node_slot_map,
				i
			);
			HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(id));
			hgraph_destroy_node(to_graph, id);
		} else {
			++i;
		}
	}
}
