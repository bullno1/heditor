#include "allocator/arena.h"
#include "hgraph/common.h"
#include "hgraph/plugin.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <hgraph/runtime.h>
#include <node_type_menu.h>
#include <cnode-editor.h>
#include <float.h>
#include "command.h"
#include "plugin_api_impl.h"
#include "utils.h"

typedef struct {
	node_type_menu_entry_t* first_entry;
	ImVec2 node_pos;
} create_node_menu_ctx_t;

typedef struct {
	hed_arena_t* arena;
	hgraph_t* graph;
	hgraph_index_t popup_node;
	const hgraph_attribute_description_t* popup_attribute;
	hed_gui_popup_info_t popup_info;
} draw_graph_ctx_t;

typedef struct pin_ctx_s {
	ImVec2 min;
	ImVec2 max;

	bool is_input;
	bool is_connected;

	struct pin_ctx_s* next;
} pin_info_t;

static void
gui_draw_graph_node_impl(
	draw_graph_ctx_t* ctx,
	hgraph_index_t node_id,
	const hgraph_node_type_t* node_type
) {
	float node_border_width;
	neGetStyleVarFloat(neStyleVar_NodeBorderWidth, &node_border_width);
	float half_border_width = node_border_width * 0.5f;
	ImVec4 node_padding;
	neGetStyleVarVec4(neStyleVar_NodePadding, &node_padding);
	const float pin_height = igGetTextLineHeight();

	ImVec2 header_min, header_max;
	ImVec2 attr_min, attr_max;
	ImVec2 input_min, input_max;
	ImVec2 output_min, output_max;
	float io_gap;
	pin_info_t* pins = NULL;
	hed_gui_popup_info_t popup_info = { 0 };
	neBeginNode(node_id);
	{
		// Header
		igBeginGroup();
		{
			igText(node_type->label.data);
			igSpacing();
		}
		igEndGroup();
		igGetItemRectMin(&header_min);
		igGetItemRectMax(&header_max);
		float header_width = header_max.x - header_min.x;

		// Attributes
		if (node_type->attributes != NULL) {
			float input_label_size = 0.f;
			for (
				const hgraph_attribute_description_t** itr = node_type->attributes;
				itr != NULL && *itr != NULL;
				++itr
			) {
				ImVec2 text_size;
				hgraph_str_t text = (*itr)->label;
				igCalcTextSize(
					&text_size,
					text.data, text.data + text.length,
					false,
					FLT_MAX
				);
				input_label_size = HED_MAX(text_size.x,  input_label_size);
			}

			igPushID_Str("attributes");
			igBeginGroup();
			for (
				const hgraph_attribute_description_t** itr = node_type->attributes;
				itr != NULL && *itr != NULL;
				++itr
			) {
				hgraph_data_render_callback_t render = NULL;
				if ((*itr)->render != NULL) {
					render = (*itr)->render;
				} else if ((*itr)->data_type->render != NULL) {
					render = (*itr)->data_type->render;
				}

				igBeginGroup();
				{
					if (render != NULL) { igSpacing(); }
					ImVec2 label_min, label_max;
					igText((*itr)->label.data);
					igGetItemRectMin(&label_min);
					igGetItemRectMax(&label_max);
					igSameLine(0.f, -1.f);
					igDummy((ImVec2){
						.x = input_label_size - (label_max.x - label_min.x),
					});
				}
				igEndGroup();

				if (render != NULL) {
					igSameLine(0.f, -1.f);
					igPushID_Int(itr - node_type->attributes);
					igBeginGroup();
					{
						hed_plugin_api_impl_t impl;
						void* attr = hgraph_get_node_attribute(ctx->graph, node_id, *itr);
						HED_WITH_ARENA(ctx->arena) {
							render(
								attr,
								hed_gui_init(
									&impl,
									&popup_info,
									false,
									ctx->arena,
									igGetCurrentContext()
								)
							);
						}
					}
					igEndGroup();
					igPopID();

					if (
						hed_gui_popup_requested(&popup_info)
						&& ctx->popup_attribute == NULL
					) {
						ctx->popup_node = node_id;
						ctx->popup_attribute = *itr;
						ctx->popup_info = popup_info;
					}
				}
			}
			igSpacing();
			igEndGroup();
			igPopID();
		}
		igGetItemRectMin(&attr_min);
		igGetItemRectMax(&attr_max);
		float attr_width = attr_max.x - attr_min.x;

		// Input pins
		igBeginGroup();
		for (
			const hgraph_pin_description_t** itr = node_type->input_pins;
			itr != NULL && *itr != NULL;
			++itr
		) {
			hgraph_index_t pin_id = hgraph_get_pin_id(ctx->graph, node_id, *itr);
			neBeginPin(pin_id, true);
			{
				igDummy((ImVec2){ node_padding.x, pin_height });
				ImVec2 min, max;
				igGetItemRectMin(&min);
				igGetItemRectMax(&max);
				min.x -= node_padding.x - half_border_width;
				min.y += node_border_width;
				max.x -= node_border_width * 2;
				max.y -= node_border_width;
				nePinRect(min, max);

				igSameLine(0.f, 0.f);

				igText((*itr)->label.data);

				// Record coordinates so we can draw them later
				pin_info_t* pin_info = HED_ARENA_ALLOC_TYPE(ctx->arena, pin_info_t);
				pin_info->min = min;
				pin_info->max = max;
				pin_info->is_input = true;
				pin_info->next = pins;
				pin_info->is_connected = hgraph_is_pin_connected(ctx->graph, pin_id);
				pins = pin_info;
			}
			neEndPin();
		}
		igEndGroup();
		igGetItemRectMin(&input_min);
		igGetItemRectMax(&input_max);
		float input_width = input_max.x - input_min.x;

		// Decide on how to space output pins
		float width = 0.f;
		width = width > header_width ? width : header_width;
		width = width > attr_width ? width : attr_width;
		width = width > input_width ? width : input_width;
		float output_block_size = 0.f;
		for (
			const hgraph_pin_description_t** itr = node_type->output_pins;
			itr != NULL && *itr != NULL;
			++itr
		) {
			ImVec2 text_size;
			hgraph_str_t text = (*itr)->label;
			igCalcTextSize(
				&text_size,
				text.data, text.data + text.length,
				false,
				FLT_MAX
			);
			output_block_size = HED_MAX(text_size.x,  output_block_size);
		}
		io_gap = node_padding.z + node_padding.x;  // Right of input, left of output
		float extra_space = io_gap + node_padding.z; // Right of output
		if (input_width + output_block_size + extra_space > width) {
			// Let it grow automatically
			igSameLine(input_width + io_gap, -1.f);
		} else {
			// Dictate width
			igSameLine(width - output_block_size - node_padding.z, -1.f);
		}

		// Output pins
		igBeginGroup();
		for (
			const hgraph_pin_description_t** itr = node_type->output_pins;
			itr != NULL && *itr != NULL;
			++itr
		) {
			hgraph_index_t pin_id = hgraph_get_pin_id(ctx->graph, node_id, *itr);
			neBeginPin(pin_id, false);
			{
				ImVec2 text_size;
				hgraph_str_t text = (*itr)->label;
				igCalcTextSize(
					&text_size,
					text.data, text.data + text.length,
					false,
					FLT_MAX
				);

				// Padding to right align label
				igDummy((ImVec2){ output_block_size - text_size.x, 0.f });
				igSameLine(0.f, 0.f);
				igText((*itr)->label.data);
				igSameLine(0.f, 0.f);
				igDummy((ImVec2){ node_padding.z, pin_height });
				ImVec2 min, max;
				igGetItemRectMin(&min);
				igGetItemRectMax(&max);
				min.x += node_border_width * 2.f;
				min.y += node_border_width;
				max.x += node_padding.z - half_border_width;
				max.y -= node_border_width;
				nePinRect(min, max);

				// Record coordinates so we can draw them later
				pin_info_t* pin_info = HED_ARENA_ALLOC_TYPE(ctx->arena, pin_info_t);
				pin_info->min = min;
				pin_info->max = max;
				pin_info->is_input = false;
				pin_info->next = pins;
				pin_info->is_connected = hgraph_is_pin_connected(ctx->graph, pin_id);
				pins = pin_info;
			}
			neEndPin();
		}
		igEndGroup();
		igGetItemRectMin(&output_min);
		igGetItemRectMax(&output_max);
	}
	neEndNode();

	// Extra decoration
	if (igIsItemVisible()) {
		ImVec2 node_min, node_max;
		igGetItemRectMin(&node_min);
		igGetItemRectMax(&node_max);

		ImVec4 border_color;
		neGetStyleColor(neStyleColor_NodeBorder, &border_color);
		ImDrawList* draw_list = neGetNodeBackgroundDrawList(node_id);

		// Divide header and attribute
		ImDrawList_AddLine(
			draw_list,
			(ImVec2){ node_min.x + half_border_width, header_max.y },
			(ImVec2){ node_max.x - node_border_width, header_max.y },
			igColorConvertFloat4ToU32(border_color),
			1.f
		);

		// Divide attribute and input/output
		ImDrawList_AddLine(
			draw_list,
			(ImVec2){ node_min.x + half_border_width, attr_max.y },
			(ImVec2){ node_max.x - node_border_width, attr_max.y },
			igColorConvertFloat4ToU32(border_color),
			1.f
		);

		// Divide input and output
		if (node_type->input_pins != NULL && node_type->output_pins != NULL) {
			float gap = output_min.x - input_max.x;
			ImDrawList_AddLine(
				draw_list,
				(ImVec2){ input_max.x + gap * 0.5f, attr_max.y + half_border_width },
				(ImVec2){ input_max.x + gap * 0.5f, node_max.y - half_border_width },
				igColorConvertFloat4ToU32(border_color),
				1.f
			);
		}

		// Pin boxes
		ImVec4 pin_color = { 1.f, 1.f, 1.f, 1.f };
		float pin_rounding;
		neGetStyleVarFloat(neStyleVar_PinRounding, &pin_rounding);
		for (
			pin_info_t* pin_info = pins;
			pin_info != NULL;
			pin_info = pin_info->next
		) {
			if (pin_info->is_connected) {
				ImDrawList_AddRectFilled(
					draw_list,
					pin_info->min,
					pin_info->max,
					igColorConvertFloat4ToU32(pin_color),
					pin_rounding,
					pin_info->is_input
						?  ImDrawFlags_RoundCornersRight
						: ImDrawFlags_RoundCornersLeft
				);
			} else {
				ImDrawList_AddRect(
					draw_list,
					pin_info->min,
					pin_info->max,
					igColorConvertFloat4ToU32(pin_color),
					pin_rounding,
					pin_info->is_input
						?  ImDrawFlags_RoundCornersRight
						: ImDrawFlags_RoundCornersLeft,
					node_border_width
				);
			}
		}
	}
}

static bool
gui_draw_graph_node(
	hgraph_index_t node_id,
	const hgraph_node_type_t* node_type,
	void* userdata
) {
	draw_graph_ctx_t* ctx = userdata;
	igPushID_Int(node_id);
	HED_WITH_ARENA(ctx->arena) {
		gui_draw_graph_node_impl(ctx, node_id, node_type);
	}
	igPopID();

	return true;
}

static bool
gui_draw_graph_edge(
	hgraph_index_t edge,
	hgraph_index_t from_pin,
	hgraph_index_t to_pin,
	void* userdata
) {
	(void)userdata;

	neLink(edge, from_pin, to_pin);
	return true;
}

static void
show_create_node_menu(
	const create_node_menu_ctx_t* ctx,
	node_type_menu_entry_t* entry
) {
	const hgraph_node_type_t* node_type = entry->node_type;
	if (node_type != NULL) {
		igPushID_Int(entry - ctx->first_entry);
		if (igMenuItem_Bool(node_type->label.data, NULL, false, true)) {
			HED_CMD(HED_CMD_CREATE_NODE, .create_node_args = {
				.node_type = node_type,
				.pos = ctx->node_pos,
			});
		}
		igPopID();
		if (
			node_type->description.length > 0
			&& igIsItemHovered(ImGuiHoveredFlags_DelayShort)
		) {
			igSetTooltip(node_type->description.data);
		}
	} else if (entry->label.length > 0) {
		if (igBeginMenu(entry->label.data, true)) {
			for (
				node_type_menu_entry_t* itr = entry->children;
				itr != NULL;
				itr = itr->next_sibling
			) {
				show_create_node_menu(ctx, itr);
			}

			igEndMenu();
		}
	} else {
		for (
			node_type_menu_entry_t* itr = entry->children;
			itr != NULL;
			itr = itr->next_sibling
		) {
			show_create_node_menu(ctx, itr);
		}
	}
}

void
draw_editor(
	hed_arena_t* arena,
	node_type_menu_entry_t* node_type_menu,
	hgraph_t* graph
) {
	draw_graph_ctx_t draw_ctx = {
		.arena = arena,
		.graph = graph,
	};
	hgraph_iterate_nodes(graph, gui_draw_graph_node, &draw_ctx);
	hgraph_iterate_edges(graph, gui_draw_graph_edge, NULL);
	const ImVec4 accept_color = { 0.f, 1.f, 0.f, 1.f };
	const ImVec4 reject_color = { 1.f, 0.f, 0.f, 1.f };

	// Edit graph
	if (neBeginCreate()) {
		hgraph_index_t from_pin, to_pin;
		if (neQueryNewLink(&from_pin, &to_pin)) {
			if (hgraph_can_connect(graph, from_pin, to_pin)) {
				if (neAcceptNewItemEx(accept_color, 1.f)) {
					HED_CMD(HED_CMD_CREATE_EDGE, .create_edge_args = {
						.from_pin = from_pin,
						.to_pin = to_pin,
					});
				}
			} else {
				neRejectNewItemEx(reject_color, 1.f);
			}
		}
	}
	neEndCreate();

	if (neBeginDelete()) {
		hgraph_index_t node_id;
		while (neQueryDeletedNode(&node_id)) {
			if (neAcceptDeletedItem(true)) {
				hgraph_destroy_node(graph, node_id);
			}
		}

		hgraph_index_t edge_id;
		while (neQueryDeletedLink(&edge_id)) {
			if (neAcceptDeletedItem(true)) {
				hgraph_disconnect(graph, edge_id);
			}
		}
	}
	neEndDelete();

	// Menu
	ImVec2 tmp_pos;
	igGetMousePos(&tmp_pos);
	neSuspend();
	{
		static ImVec2 mouse_pos;
		if (neShowBackgroundContextMenu()) {
			igOpenPopup_Str("New node", ImGuiPopupFlags_None);
			mouse_pos = tmp_pos;
		}

		if (igBeginPopup("New node", ImGuiWindowFlags_None)) {
			show_create_node_menu(
				&(create_node_menu_ctx_t){
					.first_entry = node_type_menu,
					.node_pos = mouse_pos,
				},
				node_type_menu
			);
			igEndPopup();
		}

		static hgraph_index_t popup_node = HGRAPH_INVALID_INDEX;
		static const hgraph_attribute_description_t* popup_attribute = NULL;
		static hed_gui_popup_info_t popup_info = { 0 };
		if (draw_ctx.popup_attribute) {
			igOpenPopup_Str("Edit attribute", ImGuiInputTextFlags_None);
			popup_node = draw_ctx.popup_node;
			popup_attribute = draw_ctx.popup_attribute;
			popup_info = draw_ctx.popup_info;
		}

		// Revalidate popup in case of reload
		const hgraph_node_type_t* node_type = hgraph_get_node_type(graph, popup_node);
		bool is_attribute_valid = false;
		if (node_type != NULL) {
			for (
				const hgraph_attribute_description_t** itr = node_type->attributes;
				itr != NULL && *itr != NULL;
				++itr
			) {
				if (*itr == popup_attribute) {
					is_attribute_valid = true;
					break;
				}
			}
		}
		if (!is_attribute_valid) {
			popup_node = HGRAPH_INVALID_INDEX;
			popup_attribute = NULL;
		}

		if (
			popup_attribute != NULL
			&& igBeginPopup("Edit attribute", ImGuiWindowFlags_None)
		) {
			hgraph_data_render_callback_t render = NULL;
			if (popup_attribute->render != NULL) {
				render = popup_attribute->render;
			} else if (popup_attribute->data_type->render != NULL) {
				render = popup_attribute->data_type->render;
			}

			if (render) {
				hed_plugin_api_impl_t impl;
				void* attr = hgraph_get_node_attribute(graph, popup_node, popup_attribute);

				HED_WITH_ARENA(arena) {
					render(
						attr,
						hed_gui_init(
							&impl,
							&popup_info,
							true,
							arena,
							igGetCurrentContext()
						)
					);
				}
			}

			igEndPopup();
		}
	}
	neResume();
}
