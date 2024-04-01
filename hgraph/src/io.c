#include "hgraph/runtime.h"
#include "internal.h"
#include <hgraph/io.h>
#include "graph.h"
#include "slip.h"

const char HEADER_MAGIC_V1[] = { 'H', 'E', 'D', 1 };
const size_t HEADER_MAGIC_SIZE = sizeof(HEADER_MAGIC_V1);

HGRAPH_PRIVATE hgraph_io_status_t
hgraph_write_graph_v1(const hgraph_t* graph, hgraph_out_t* out) {
	hgraph_index_t num_nodes = graph->node_slot_map.num_items;
	HGRAPH_CHECK_IO(hgraph_io_write_uint(num_nodes, out));

	hgraph_index_t max_name_length = 0;
	for (hgraph_index_t i = 0; i < num_nodes; ++i) {
		const hgraph_node_t* node = hgraph_get_node_by_slot(graph, i);
		hgraph_str_t name = hgraph_get_node_name_internal(graph, node);
		max_name_length = HGRAPH_MAX(max_name_length, name.length);
	}
	HGRAPH_CHECK_IO(hgraph_io_write_uint(max_name_length, out));

	HGRAPH_CHECK_IO(hgraph_io_write_uint(num_nodes, out));
	for (hgraph_index_t i = 0; i < num_nodes; ++i) {
		const hgraph_node_t* node = hgraph_get_node_by_slot(graph, i);

		const hgraph_node_type_info_t* type_info = hgraph_get_node_type_internal(
			graph, node
		);
		HGRAPH_CHECK_IO(hgraph_io_write_str(type_info->name, out));

		hgraph_str_t name = hgraph_get_node_name_internal(graph, node);
		HGRAPH_CHECK_IO(hgraph_io_write_str(name, out));

		HGRAPH_CHECK_IO(hgraph_io_write_uint(type_info->num_attributes, out));
		for (hgraph_index_t j = 0; j < type_info->num_attributes; ++j) {
			const hgraph_var_t* var = &type_info->attributes[j];
			const hgraph_data_type_info_t* data_type = &graph->registry->data_types[var->type];

			HGRAPH_CHECK_IO(hgraph_io_write_str(data_type->name, out));
			HGRAPH_CHECK_IO(hgraph_io_write_str(var->name, out));

			const void* value = (char*)node + var->offset;
			hgraph_slip_out_t slip;
			hgraph_out_t* wrapped_out = hgraph_slip_out_init(&slip, out);
			HGRAPH_CHECK_IO(data_type->definition->serialize(value, wrapped_out));
			HGRAPH_CHECK_IO(hgraph_slip_out_end(out));
		}
	}

	hgraph_index_t num_edges = graph->edge_slot_map.num_items;
	HGRAPH_CHECK_IO(hgraph_io_write_uint(num_edges, out));
	for (hgraph_index_t i = 0; i < num_edges; ++i) {
		const hgraph_edge_t* edge = &graph->edges[i];

		bool is_output;
		hgraph_index_t from_node_id, from_pin_index;
		hgraph_decode_pin_id(edge->from_pin, &from_node_id, &from_pin_index, &is_output);

		hgraph_index_t to_node_id, to_pin_index;
		hgraph_decode_pin_id(edge->to_pin, &to_node_id, &to_pin_index, &is_output);

		hgraph_node_t* from_node = hgraph_find_node_by_id(graph, from_node_id);
		hgraph_node_t* to_node = hgraph_find_node_by_id(graph, to_node_id);

		const hgraph_node_type_info_t* from_type_info = hgraph_get_node_type_internal(
			graph, from_node
		);
		const hgraph_node_type_info_t* to_type_info = hgraph_get_node_type_internal(
			graph, to_node
		);

		hgraph_index_t from_node_slot = ((char*)from_node - graph->nodes) / graph->node_size;
		hgraph_index_t to_node_slot = ((char*)to_node - graph->nodes) / graph->node_size;

		HGRAPH_CHECK_IO(hgraph_io_write_uint(from_node_slot, out));
		HGRAPH_CHECK_IO(hgraph_io_write_str(from_type_info->output_pins[from_pin_index].name, out));

		HGRAPH_CHECK_IO(hgraph_io_write_uint(to_node_slot, out));
		HGRAPH_CHECK_IO(hgraph_io_write_str(to_type_info->input_pins[to_pin_index].name, out));
	}

	return HGRAPH_IO_OK;
}

HGRAPH_PRIVATE hgraph_io_status_t
hgraph_read_graph_config_v1(hgraph_config_t* config, hgraph_in_t* in) {
	uint64_t num_nodes;
	uint64_t max_name_length;

	HGRAPH_CHECK_IO(hgraph_io_read_uint(&num_nodes, in));
	HGRAPH_CHECK_IO(hgraph_io_read_uint(&max_name_length, in));

	config->max_nodes = num_nodes;
	config->max_name_length = max_name_length;
	return HGRAPH_IO_OK;
}

HGRAPH_PRIVATE hgraph_io_status_t
hgraph_read_graph_v1(hgraph_t* graph, hgraph_in_t* in) {
	hgraph_index_t num_node_types = graph->registry->num_node_types;
	const hgraph_node_type_info_t* node_types = graph->registry->node_types;
	const hgraph_data_type_info_t* data_types = graph->registry->data_types;

	uint64_t num_nodes_varint;
	HGRAPH_CHECK_IO(hgraph_io_read_uint(&num_nodes_varint, in));
	hgraph_index_t num_nodes = num_nodes_varint;
	for (hgraph_index_t i = 0; i < num_nodes; ++i) {
		char type_buf[256];
		char name_buf[256];
		size_t len;
		hgraph_str_t type = { .data = type_buf };
		hgraph_str_t name = { .data = name_buf };

		len = sizeof(type_buf);
		HGRAPH_CHECK_IO(hgraph_io_read_str(type_buf, &len, in));
		type.length = len;

		const hgraph_node_type_info_t* node_type_info = &node_types[0];
		for (hgraph_index_t j = 1; j < num_node_types; ++j) {
			if (hgraph_str_equal(node_types[j].name, type)) {
				node_type_info = &node_types[j];
				break;
			}
		}

		hgraph_index_t node_id = hgraph_create_node(graph, node_type_info->definition);
		if (!HGRAPH_IS_VALID_INDEX(node_id)) { return HGRAPH_IO_MALFORMED; }

		hgraph_node_t* node = hgraph_find_node_by_id(graph, node_id);
		char* name_storage = (char*)node + node_type_info->size;
		node->name_len = graph->max_name_length;
		HGRAPH_CHECK_IO(hgraph_io_read_str(name_storage, &node->name_len, in));

		uint64_t num_attributes_varint;
		HGRAPH_CHECK_IO(hgraph_io_read_uint(&num_attributes_varint, in));
		hgraph_index_t num_attributes = num_attributes_varint;
		for (hgraph_index_t j = 0; j < num_attributes; ++j) {
			len = sizeof(type_buf);
			HGRAPH_CHECK_IO(hgraph_io_read_str(type_buf, &len, in));
			type.length = len;

			len = sizeof(name_buf);
			HGRAPH_CHECK_IO(hgraph_io_read_str(name_buf, &len, in));
			name.length = len;

			hgraph_index_t num_attributes = node_type_info->num_attributes;
			const hgraph_var_t* attribute = NULL;
			const hgraph_data_type_info_t* attribute_type = NULL;
			for (hgraph_index_t k = 0; k < num_attributes; ++k) {
				const hgraph_var_t* var = &node_type_info->attributes[k];
				if (hgraph_str_equal(var->name, name)) {
					const hgraph_data_type_info_t* var_type = &data_types[var->type];

					if (hgraph_str_equal(var_type->name, type)) {
						attribute = var;
						attribute_type = var_type;
					}

					// If the type does not match, ignore this attribute
					break;
				}
			}

			if (attribute != NULL) {
				hgraph_slip_in_t slip;
				hgraph_in_t* wrapped_in = hgraph_slip_in_init(&slip, in);
				void* value = (char*)node + attribute->offset;
				HGRAPH_CHECK_IO(attribute_type->definition->deserialize(value, wrapped_in));
				HGRAPH_CHECK_IO(hgraph_slip_in_end(in));
			} else {
				HGRAPH_CHECK_IO(hgraph_slip_in_skip(in));
			}
		}
	}

	uint64_t num_edges_varint;
	HGRAPH_CHECK_IO(hgraph_io_read_uint(&num_edges_varint, in));
	hgraph_index_t num_edges = num_edges_varint;
	for (hgraph_index_t i = 0; i < num_edges; ++i) {
		char from_name_buf[256];
		char to_name_buf[256];
		size_t len;
		hgraph_str_t from_pin_name = { .data = from_name_buf };
		hgraph_str_t to_pin_name = { .data = to_name_buf };
		uint64_t from_node_slot, to_node_slot;

		HGRAPH_CHECK_IO(hgraph_io_read_uint(&from_node_slot, in));
		len = sizeof(from_name_buf);
		HGRAPH_CHECK_IO(hgraph_io_read_str(from_name_buf, &len, in));
		from_pin_name.length = len;

		HGRAPH_CHECK_IO(hgraph_io_read_uint(&to_node_slot, in));
		len = sizeof(to_name_buf);
		HGRAPH_CHECK_IO(hgraph_io_read_str(to_name_buf, &len, in));
		to_pin_name.length = len;

		if (
			(from_node_slot >= (uint64_t)graph->node_slot_map.num_items)
			|| (to_node_slot >= (uint64_t)graph->node_slot_map.num_items)
		) {
			return HGRAPH_IO_MALFORMED;
		}

		hgraph_node_t* from_node = hgraph_get_node_by_slot(graph, from_node_slot);
		hgraph_node_t* to_node = hgraph_get_node_by_slot(graph, to_node_slot);

		const hgraph_node_type_info_t* from_node_type = &node_types[from_node->type];
		const hgraph_node_type_info_t* to_node_type = &node_types[to_node->type];

		hgraph_index_t from_pin_index = HGRAPH_INVALID_INDEX;
		for (hgraph_index_t j = 0; j < from_node_type->num_output_pins; ++j) {
			const hgraph_var_t* pin = &from_node_type->output_pins[j];
			if (hgraph_str_equal(pin->name, from_pin_name)) {
				from_pin_index = j;
				break;
			}
		}
		if (!HGRAPH_IS_VALID_INDEX(from_pin_index)) { continue; }

		hgraph_index_t to_pin_index = HGRAPH_INVALID_INDEX;
		for (hgraph_index_t j = 0; j < to_node_type->num_input_pins; ++j) {
			const hgraph_var_t* pin = &to_node_type->input_pins[j];
			if (hgraph_str_equal(pin->name, from_pin_name)) {
				to_pin_index = j;
				break;
			}
		}
		if (!HGRAPH_IS_VALID_INDEX(to_pin_index)) { continue; }

		hgraph_index_t from_node_id = hgraph_slot_map_id_for_slot(
			&graph->node_slot_map,
			from_node_slot
		);
		hgraph_index_t to_node_id = hgraph_slot_map_id_for_slot(
			&graph->node_slot_map,
			to_node_slot
		);

		hgraph_index_t from_pin = hgraph_encode_pin_id(from_node_id, from_pin_index, true);
		hgraph_index_t to_pin = hgraph_encode_pin_id(to_node_id, to_pin_index, false);

		hgraph_connect(graph, from_pin, to_pin);
	}

	// Remove dummy nodes
	for (hgraph_index_t i = 0; i < graph->node_slot_map.num_items;) {
		const hgraph_node_t* node = hgraph_get_node_by_slot(graph, i);
		if (node->type == 0) {
			hgraph_index_t id = hgraph_slot_map_id_for_slot(
				&graph->node_slot_map,
				i
			);
			HGRAPH_ASSERT(HGRAPH_IS_VALID_INDEX(id));
			hgraph_destroy_node(graph, id);
		} else {
			++i;
		}
	}

	return HGRAPH_IO_OK;
}

hgraph_io_status_t
hgraph_write_header(hgraph_out_t* out) {
	return hgraph_io_write(out, HEADER_MAGIC_V1, HEADER_MAGIC_SIZE);
}

hgraph_io_status_t
hgraph_read_header(hgraph_header_t* header, hgraph_in_t* in) {
	char magic[HEADER_MAGIC_SIZE];
	HGRAPH_CHECK_IO(hgraph_io_read(in, magic, HEADER_MAGIC_SIZE));

	if (memcmp(magic, HEADER_MAGIC_V1, HEADER_MAGIC_SIZE) == 0) {
		header->version = 1;
	} else {
		return HGRAPH_IO_MALFORMED;
	}

	return HGRAPH_IO_OK;
}

hgraph_io_status_t
hgraph_write_graph(const hgraph_t* graph, hgraph_out_t* out) {
	return hgraph_write_graph_v1(graph, out);
}

hgraph_io_status_t
hgraph_read_graph_config(
	const hgraph_header_t* header,
	hgraph_config_t* config,
	hgraph_in_t* in
) {
	switch (header->version) {
		case 1:
			return hgraph_read_graph_config_v1(config, in);
		default:
			return HGRAPH_IO_MALFORMED;
	}
}

hgraph_io_status_t
hgraph_read_graph(
	const hgraph_header_t* header,
	hgraph_t* graph,
	hgraph_in_t* in
) {
	switch (header->version) {
		case 1:
			return hgraph_read_graph_v1(graph, in);
		default:
			return HGRAPH_IO_MALFORMED;
	}
}
