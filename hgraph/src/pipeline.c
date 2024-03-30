#include "internal.h"
#include "graph.h"
#include "mem_layout.h"
#include "slot_map.h"

HGRAPH_API hgraph_pipeline_t*
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
		node_data_pool = (char*)mem_layout_align_ptr((intptr_t)node_data_pool, _Alignof(max_align_t));

		if (node_type->definition->init != NULL) {
			node_type->definition->init(node_meta->data);
		} else {
			memset(node_meta->data, 0, node_type->definition->size);
		}

		for (hgraph_index_t j = 0; j < node_type->num_output_pins; ++j) {
			const hgraph_pin_description_t* pin_def = node_type->definition->output_pins[j];
			char* output_buf = node_meta->data + node_type->output_buffers[j].offset;
			if (pin_def->data_type->init != NULL) {
				pin_def->data_type->init(output_buf);
			}
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

HGRAPH_API void
hgraph_pipeline_cleanup(hgraph_pipeline_t* pipeline) {
	for (hgraph_index_t i = 0; i < pipeline->num_nodes; ++i) {
		hgraph_pipeline_node_meta_t* node_meta = &pipeline->node_metas[i];
		const hgraph_node_type_info_t* node_type = &pipeline->graph->registry->node_types[node_meta->type];

		if (node_type->definition->cleanup != NULL) {
			node_type->definition->cleanup(node_meta->data);
		}
	}
}

HGRAPH_API hgraph_pipeline_execution_status_t
hgraph_pipeline_execute(
	hgraph_pipeline_t* pipeline,
	hgraph_pipeline_watcher_t watcher,
	void* userdata
);
