#include "internal.h"
#include "graph.h"
#include "mem_layout.h"
#include "slot_map.h"

typedef struct hgraph_pipeline_node_ctx_s {
	hgraph_node_api_t impl;
	hgraph_pipeline_t* pipeline;
	hgraph_index_t slot;
	hgraph_pipeline_watcher_t watcher;
	void* watcher_data;
	hgraph_pipeline_execution_status_t termination_reason;
} hgraph_pipeline_node_ctx_t;

HGRAPH_PRIVATE void*
hgraph_pipeline_node_allocate(
	const hgraph_node_api_t* api,
	hgraph_lifetime_t lifetime,
	size_t size
) {
	(void)lifetime;
	(void)size;
	hgraph_pipeline_node_ctx_t* ctx = HGRAPH_CONTAINER_OF(api, hgraph_pipeline_node_ctx_t, impl);
	ctx->termination_reason = HGRAPH_PIPELINE_EXEC_OOM;
	return NULL;
}

HGRAPH_PRIVATE void*
hgraph_pipeline_node_data(const hgraph_node_api_t* api) {
	hgraph_pipeline_node_ctx_t* ctx = HGRAPH_CONTAINER_OF(api, hgraph_pipeline_node_ctx_t, impl);
	hgraph_pipeline_t* pipeline = ctx->pipeline;
	hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[ctx->slot];
	return node_meta->data;
}

HGRAPH_PRIVATE void
hgraph_pipeline_node_report_status(const hgraph_node_api_t* api, const void* status) {
	hgraph_pipeline_node_ctx_t* ctx = HGRAPH_CONTAINER_OF(api, hgraph_pipeline_node_ctx_t, impl);
	hgraph_pipeline_t* pipeline = ctx->pipeline;
	hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[ctx->slot];
	node_meta->status = status;

	if (!ctx->watcher(
		&(hgraph_pipeline_event_t){
			.type = HGRAPH_PIPELINE_EV_UPDATE_STATUS,
			.node = node_meta->id,
		},
		ctx->watcher_data
	)) {
		ctx->termination_reason = HGRAPH_PIPELINE_EXEC_ABORTED;
	}
}

HGRAPH_PRIVATE const void*
hgraph_pipeline_node_input(
	const hgraph_node_api_t* api,
	const void* pinOrAttribute
) {
	hgraph_pipeline_node_ctx_t* ctx = HGRAPH_CONTAINER_OF(api, hgraph_pipeline_node_ctx_t, impl);
	hgraph_pipeline_t* pipeline = ctx->pipeline;
	const hgraph_t* graph = pipeline->graph;
	hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[ctx->slot];
	const hgraph_node_type_info_t* node_type = &graph->registry->node_types[node_meta->type];
	const hgraph_node_t* node = hgraph_get_node_by_slot(graph, ctx->slot);

	for (hgraph_index_t i = 0; i < node_type->num_attributes; ++i) {
		const hgraph_attribute_description_t* attribute = node_type->definition->attributes[i];
		if (attribute == pinOrAttribute) {
			const hgraph_var_t* var = &node_type->attributes[i];
			return (char*)node + var->offset;
		}
	}

	for (hgraph_index_t i = 0; i < node_type->num_input_pins; ++i) {
		const hgraph_pin_description_t* pin = node_type->definition->input_pins[i];
		if (pin == pinOrAttribute) {
			const hgraph_var_t* var = &node_type->input_pins[i];

			hgraph_index_t edge_id = *((hgraph_index_t*)((char*)node + var->offset));
			hgraph_index_t edge_slot = hgraph_slot_map_slot_for_id(
				&graph->edge_slot_map, edge_id
			);
			if (!HGRAPH_IS_VALID_INDEX(edge_slot)) {  // No connection
				return NULL;
			}
			const hgraph_edge_t* edge = &graph->edges[edge_slot];

			hgraph_index_t from_node_id, from_pin_index;
			bool is_output;
			hgraph_decode_pin_id(edge->from_pin, &from_node_id, &from_pin_index, &is_output);
			HGRAPH_ASSERT(is_output);

			hgraph_index_t from_node_slot = hgraph_slot_map_slot_for_id(
				&graph->node_slot_map, from_node_id
			);
			HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(from_node_slot));

			const hgraph_pipeline_node_meta_t* from_node_meta = &pipeline->node_metas[from_node_slot];
			const hgraph_node_type_info_t* from_node_type = &graph->registry->node_types[from_node_meta->type];
			return from_node_meta->data + from_node_type->output_buffers[from_pin_index].offset;
		}
	}

	return NULL;
}

HGRAPH_PRIVATE void
hgraph_pipeline_node_output(
	const hgraph_node_api_t* api,
	const hgraph_pin_description_t* pin,
	const void* value
) {
	hgraph_pipeline_node_ctx_t* ctx = HGRAPH_CONTAINER_OF(api, hgraph_pipeline_node_ctx_t, impl);
	hgraph_pipeline_t* pipeline = ctx->pipeline;
	const hgraph_t* graph = pipeline->graph;
	hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[ctx->slot];
	const hgraph_node_type_info_t* node_type = &graph->registry->node_types[node_meta->type];
	const hgraph_node_t* node = hgraph_get_node_by_slot(graph, ctx->slot);

	for (hgraph_index_t i = 0; i < node_type->num_output_pins; ++i) {
		const hgraph_pin_description_t* pin_def = node_type->definition->output_pins[i];
		if (pin == pin_def) {
			// Copy value to output buffer
			char* output_addr = node_meta->data + node_type->output_buffers[i].offset;
			memcpy(output_addr, value, pin_def->data_type->size);
			hgraph_bitset_set(&node_meta->sent_outputs, i);

			// Notify all nodes that depends on this
			const hgraph_var_t* var = &node_type->output_pins[i];
			hgraph_edge_link_t* output_pin = (hgraph_edge_link_t*)((char*)node + var->offset);
			hgraph_index_t itr = output_pin->next;
			while (true) {
				const hgraph_edge_link_t* link = hgraph_resolve_edge(graph, output_pin, itr);
				if (link == output_pin) { break; }

				const hgraph_edge_t* edge = HGRAPH_CONTAINER_OF(link, hgraph_edge_t, output_pin_link);
				hgraph_index_t to_node_id, to_pin_index;
				bool is_output;
				hgraph_decode_pin_id(edge->to_pin, &to_node_id, &to_pin_index, &is_output);
				hgraph_index_t to_node_slot = hgraph_slot_map_slot_for_id(
					&graph->node_slot_map, to_node_id
				);
				HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(to_node_slot));
				hgraph_pipeline_node_meta_t* to_node_meta = &pipeline->node_metas[to_node_slot];
				hgraph_bitset_set(&to_node_meta->received_inputs, to_pin_index);

				itr = link->next;
			}
		}
	}
}

HGRAPH_PRIVATE bool
hgraph_pipeline_default_watcher(
	const hgraph_pipeline_event_t* event,
	void* userdata
) {
	(void)event;
	(void)userdata;

	return true;
}

static const hgraph_node_api_t hgraph_pipeline_node_api = {
	.allocate = hgraph_pipeline_node_allocate,
	.data = hgraph_pipeline_node_data,
	.input = hgraph_pipeline_node_input,
	.output = hgraph_pipeline_node_output,
	.report_status = hgraph_pipeline_node_report_status,
};

static const hgraph_node_api_t hgraph_pipeline_node_api_no_io = {
	.allocate = hgraph_pipeline_node_allocate,
	.data = hgraph_pipeline_node_data,
	.report_status = hgraph_pipeline_node_report_status,
};

HGRAPH_PRIVATE bool
hgraph_pipeline_schedule_node(
	hgraph_pipeline_t* pipeline, hgraph_index_t node_slot
) {
	hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[node_slot];
	if (node_meta->state != HGRAPH_NODE_STATE_WAITING) { return false; }

	pipeline->ready_nodes[pipeline->num_ready_nodes++] = node_slot;
	node_meta->state = HGRAPH_NODE_STATE_SCHEDULED;
	return true;
}

HGRAPH_PRIVATE bool
hgraph_pipeline_is_node_ready(
	hgraph_pipeline_t* pipeline,
	hgraph_index_t slot
) {
	const hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[slot];
	const hgraph_node_type_info_t* node_type = &pipeline->graph->registry->node_types[node_meta->type];
	return hgraph_bitset_is_all_set(node_meta->received_inputs, node_type->required_inputs);
}

hgraph_pipeline_t*
hgraph_pipeline_init(
	const hgraph_t* graph,
	const hgraph_pipeline_config_t* config,
	hgraph_pipeline_t* previous_pipeline,
	void* mem,
	size_t* mem_size_inout
) {
	mem_layout_t layout = { 0 };
	mem_layout_reserve(
		&layout,
		sizeof(hgraph_pipeline_t),
		_Alignof(hgraph_pipeline_t)
	);

	hgraph_index_t num_nodes = graph->node_slot_map.num_items;

	ptrdiff_t ready_nodes_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_index_t) * num_nodes,
		_Alignof(hgraph_index_t)
	);
	ptrdiff_t node_metas_offset = mem_layout_reserve(
		&layout,
		sizeof(hgraph_pipeline_node_meta_t) * num_nodes,
		_Alignof(hgraph_pipeline_node_meta_t)
	);
	ptrdiff_t scratch_offset = mem_layout_reserve(
		&layout,
		config->max_scratch_memory,
		_Alignof(max_align_t)
	);

	ptrdiff_t node_data_offset = mem_layout_reserve(
		&layout, 0, _Alignof(max_align_t)
	);
	for (hgraph_index_t i = 0; i < num_nodes; ++i) {
		const hgraph_node_t* node = hgraph_get_node_by_slot(graph, i);
		const hgraph_node_type_info_t* node_type = hgraph_get_node_type_internal(
			graph, node
		);

		mem_layout_reserve(
			&layout,
			node_type->pipeline_data_size,
			_Alignof(max_align_t)
		);
	}

	size_t required_size = mem_layout_size(&layout);
	if (mem == NULL || required_size > *mem_size_inout) {
		*mem_size_inout = required_size;
		return NULL;
	}

	hgraph_pipeline_t* pipeline = mem;
	*pipeline = (hgraph_pipeline_t){
		.graph = graph,
		.num_nodes = num_nodes,
		.ready_nodes = mem_layout_locate(pipeline, ready_nodes_offset),
		.node_metas = mem_layout_locate(pipeline, node_metas_offset),
		.scratch_zone_start = mem_layout_locate(pipeline, scratch_offset),
	};
	pipeline->scratch_zone_end = pipeline->scratch_zone_start + config->max_scratch_memory;

	char* node_data_pool = mem_layout_locate(pipeline, node_data_offset);
	for (hgraph_index_t i = 0; i < num_nodes; ++i) {
		hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[i];
		const hgraph_node_t* node = hgraph_get_node_by_slot(graph, i);
		const hgraph_node_type_info_t* node_type = hgraph_get_node_type_internal(
			graph, node
		);

		hgraph_index_t node_id = hgraph_slot_map_id_for_slot(&graph->edge_slot_map, i);
		*node_meta = (hgraph_pipeline_node_meta_t){
			.id = node_id,
			.type = node->type,
			.version = graph->node_versions[node_id],
		};

		node_meta->data = node_data_pool;
		node_data_pool += node_type->pipeline_data_size;
		node_data_pool = (char*)mem_layout_align_ptr(
			(intptr_t)node_data_pool, _Alignof(max_align_t)
		);

		if (node_type->definition->init != NULL) {
			node_type->definition->init(node_meta->data);
		} else {
			memset(node_meta->data, 0, node_type->definition->size);
		}

		if (
			node_type->definition->transfer != NULL
			&& previous_pipeline != NULL
		) {
			for (hgraph_index_t j = 0; j < previous_pipeline->num_nodes; ++j) {
				hgraph_pipeline_node_meta_t* previous_node_meta =
					&previous_pipeline->node_metas[j];

				if (
					previous_node_meta->id == node_meta->id
					&& previous_node_meta->version == node_meta->version
				) {
					node_type->definition->transfer(
						node_meta->data,
						previous_node_meta->data
					);
					break;
				}
			}
		}
	}

	*mem_size_inout = required_size;
	return pipeline;
}

void
hgraph_pipeline_cleanup(hgraph_pipeline_t* pipeline) {
	for (hgraph_index_t i = 0; i < pipeline->num_nodes; ++i) {
		hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[i];
		const hgraph_node_type_info_t* node_type = &pipeline->graph->registry->node_types[node_meta->type];

		if (node_type->definition->cleanup != NULL) {
			node_type->definition->cleanup(node_meta->data);
		}
	}
}

hgraph_pipeline_execution_status_t
hgraph_pipeline_execute(
	hgraph_pipeline_t* pipeline,
	hgraph_pipeline_watcher_t watcher,
	void* userdata
) {
	const hgraph_t* graph = pipeline->graph;

	if (graph->version != pipeline->version) {
		return HGRAPH_PIPELINE_EXEC_OUT_OF_SYNC;
	}

	if (watcher == NULL) { watcher = hgraph_pipeline_default_watcher; }

	if (!watcher(
		&(hgraph_pipeline_event_t){
			.type = HGRAPH_PIPELINE_EV_BEGIN_PIPELINE,
		},
		userdata
	)) {
		return HGRAPH_PIPELINE_EXEC_ABORTED;
	}

	pipeline->step_alloc_ptr = pipeline->scratch_zone_start;
	pipeline->execution_alloc_ptr = pipeline->scratch_zone_end;
	pipeline->num_ready_nodes = 0;

	// Init nodes
	const hgraph_node_type_info_t* node_types = graph->registry->node_types;
	for (hgraph_index_t i = 0; i < pipeline->num_nodes; ++i) {
		hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[i];
		node_meta->state = HGRAPH_NODE_STATE_WAITING;
		node_meta->status = NULL;
		hgraph_bitset_init(&node_meta->received_inputs);
		hgraph_bitset_init(&node_meta->sent_outputs);

		// Init output buffers
		const hgraph_node_type_info_t* node_type = &node_types[node_meta->type];
		for (hgraph_index_t j = 0; j < node_type->num_output_pins; ++j) {
			const hgraph_pin_description_t* pin_def = node_type->definition->output_pins[j];
			char* output_buf = node_meta->data + node_type->output_buffers[j].offset;
			if (pin_def->data_type->init != NULL) {
				pin_def->data_type->init(output_buf);
			} else {
				memset(output_buf, 0, pin_def->data_type->size);
			}
		}

		// Call begin_pipeline
		if (node_type->definition->begin_pipeline != NULL) {
			hgraph_pipeline_node_ctx_t ctx = {
				.impl = hgraph_pipeline_node_api_no_io,
				.pipeline = pipeline,
				.watcher = watcher,
				.watcher_data = userdata,
				.slot = i,
			};
			node_type->definition->begin_pipeline(&ctx.impl);
			if (ctx.termination_reason != HGRAPH_PIPELINE_EXEC_FINISHED) {
				return ctx.termination_reason;
			}
		}
	}

	// Schedule the initial nodes
	for (hgraph_index_t i = 0; i < pipeline->num_nodes; ++i) {
		hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[i];

		if (
			hgraph_pipeline_is_node_ready(pipeline, i)
			&& hgraph_pipeline_schedule_node(pipeline, i)
			&& !watcher(
				&(hgraph_pipeline_event_t){
					.type = HGRAPH_PIPELINE_EV_SCHEDULE_NODE,
					.node = node_meta->id,
				},
				userdata
			)
		) {
			return HGRAPH_PIPELINE_EXEC_ABORTED;
		}
	}

	while (pipeline->num_ready_nodes > 0) {
		hgraph_index_t node_slot = pipeline->ready_nodes[--pipeline->num_ready_nodes];
		hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[node_slot];
		const hgraph_node_type_info_t* node_type = &node_types[node_meta->type];

		if (!watcher(
			&(hgraph_pipeline_event_t){
				.type = HGRAPH_PIPELINE_EV_BEGIN_NODE,
				.node = node_meta->id,
			},
			userdata
		)) {
			return HGRAPH_PIPELINE_EXEC_ABORTED;
		}

		HGRAPH_ASSERT(node_type->definition->execute != NULL);
		hgraph_pipeline_node_ctx_t ctx = {
			.impl = hgraph_pipeline_node_api,
			.pipeline = pipeline,
			.watcher = watcher,
			.watcher_data = userdata,
			.slot = node_slot,
		};
		node_type->definition->execute(&ctx.impl);
		node_meta->state = HGRAPH_NODE_STATE_EXECUTED;
		if (ctx.termination_reason != HGRAPH_PIPELINE_EXEC_FINISHED) {
			return ctx.termination_reason;
		}
		if (!hgraph_bitset_is_all_set(node_meta->sent_outputs, node_type->required_outputs)) {
			return HGRAPH_PIPELINE_EXEC_INCOMPLETE_OUTPUT;
		}

		// Schedule dependent nodes
		const hgraph_node_t* node = hgraph_get_node_by_slot(graph, node_slot);
		for (hgraph_index_t i = 0; i < node_type->num_output_pins; ++i) {
			const hgraph_var_t* var = &node_type->output_pins[i];
			hgraph_edge_link_t* output_pin = (hgraph_edge_link_t*)((char*)node + var->offset);
			hgraph_index_t itr = output_pin->next;
			while (true) {
				const hgraph_edge_link_t* link = hgraph_resolve_edge(graph, output_pin, itr);
				if (link == output_pin) { break; }

				const hgraph_edge_t* edge = HGRAPH_CONTAINER_OF(link, hgraph_edge_t, output_pin_link);
				hgraph_index_t to_node_id, to_pin_index;
				bool is_output;
				hgraph_decode_pin_id(edge->to_pin, &to_node_id, &to_pin_index, &is_output);
				hgraph_index_t to_node_slot = hgraph_slot_map_slot_for_id(
					&graph->node_slot_map, to_node_id
				);
				HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(to_node_slot));

				if (
					hgraph_pipeline_is_node_ready(pipeline, i)
					&& hgraph_pipeline_schedule_node(pipeline, i)
					&& !watcher(
						&(hgraph_pipeline_event_t){
							.type = HGRAPH_PIPELINE_EV_SCHEDULE_NODE,
							.node = node_meta->id,
						},
						userdata
					)
				) {
					return HGRAPH_PIPELINE_EXEC_ABORTED;
				}

				itr = link->next;
			}
		}

		if (!watcher(
			&(hgraph_pipeline_event_t){
				.type = HGRAPH_PIPELINE_EV_END_NODE,
				.node = node_meta->id,
			},
			userdata
		)) {
			return HGRAPH_PIPELINE_EXEC_ABORTED;
		}
	}

	for (hgraph_index_t i = 0; i < pipeline->num_nodes; ++i) {
		hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[i];
		const hgraph_node_type_info_t* node_type = &node_types[node_meta->type];
		if (node_type->definition->end_pipeline != NULL) {
			hgraph_pipeline_node_ctx_t ctx = {
				.impl = hgraph_pipeline_node_api_no_io,
				.pipeline = pipeline,
				.watcher = watcher,
				.watcher_data = userdata,
				.slot = i,
			};
			node_type->definition->end_pipeline(&ctx.impl);
		}
	}

	watcher(
		&(hgraph_pipeline_event_t){
			.type = HGRAPH_PIPELINE_EV_END_PIPELINE,
		},
		userdata
	);

	return HGRAPH_PIPELINE_EXEC_FINISHED;
}
