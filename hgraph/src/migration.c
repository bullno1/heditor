#include "internal.h"

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
			if (
				hgraph_str_equal(from_var->name, to_var->name)
				&& hgraph_str_equal(from_var->type, to_var->type)
			) {
				new_index = j;
				break;
			}
		}

		plans[i].new_index = new_index;
	}

	return plans;
}

hgraph_migration_t*
hgraph_migration_init(
	const hgraph_registry_t* from_registry,
	const hgraph_registry_t* to_registry,
	void* mem,
	size_t* mem_size_inout
) {
	mem_layout_t layout = { 0 };
	mem_layout_reserve(
		&layout,
		sizeof(hgraph_migration_t),
		_Alignof(hgraph_migration_t)
	);
	ptrdiff_t node_plans_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_node_migration_plan_t) * from_registry->num_node_types,
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
	if (mem == NULL || required_size > *mem_size_inout) {
		*mem_size_inout = required_size;
		return NULL;
	}

	hgraph_migration_t* migration = mem;
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
		for (hgraph_index_t j = 0; j < to_registry->num_node_types; ++j) {
			const hgraph_node_type_info_t* itr = &to_registry->node_types[i];

			if (hgraph_str_equal(from_type_info->name, itr->name)) {
				to_type_info = itr;
				break;
			}
		}

		hgraph_var_migration_plan_t* attribute_plans = hgraph_create_var_plans(
			&var_plans,
			from_type_info->attributes, from_type_info->num_attributes,
			to_type_info->attributes, to_type_info->num_attributes
		);
		hgraph_var_migration_plan_t* input_plans = hgraph_create_var_plans(
			&var_plans,
			from_type_info->input_pins, from_type_info->num_input_pins,
			to_type_info->input_pins, to_type_info->num_input_pins
		);
		hgraph_var_migration_plan_t* output_plans = hgraph_create_var_plans(
			&var_plans,
			from_type_info->output_pins, from_type_info->num_output_pins,
			to_type_info->output_pins, to_type_info->num_output_pins
		);

		migration->node_plans[i] = (hgraph_node_migration_plan_t){
			.new_type = to_type_info->definition,
			.attribute_plans = attribute_plans,
			.output_plans = output_plans,
			.input_plans = input_plans,
		};
	}

	*mem_size_inout = required_size;
	return migration;
}

void
hgraph_migration_execute(
	const hgraph_migration_t* migration,
	const hgraph_t* from_graph,
	hgraph_t* to_graph
);
