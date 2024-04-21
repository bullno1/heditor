#ifndef HGRAPH_PLUGIN_H
#define HGRAPH_PLUGIN_H

#include "common.h"

#define HGRAPH_LIST(TYPE, ...) (const TYPE*[]) { __VA_ARGS__, NULL }
#define HGRAPH_NODE_PINS(...) HGRAPH_LIST(hgraph_pin_description_t, __VA_ARGS__)
#define HGRAPH_NODE_ATTRIBUTES(...) HGRAPH_LIST(hgraph_attribute_description_t, __VA_ARGS__)

typedef struct hgraph_node_api_s hgraph_node_api_t;
typedef struct hgraph_plugin_api_s hgraph_plugin_api_t;

typedef enum hgraph_lifetime_e {
	HGRAPH_LIFETIME_STEP,
	HGRAPH_LIFETIME_EXECUTION,
} hgraph_lifetime_t;

typedef enum hgraph_node_part_e {
	HGRAPH_NODE_HEADER,
	HGRAPH_NODE_FOOTER,
} hgraph_node_part_t;

typedef enum hgraph_flow_type_e {
	HGRAPH_FLOW_ONCE,
	HGRAPH_FLOW_OPTIONAL,
	HGRAPH_FLOW_STREAMING,
} hgraph_flow_type_t;

typedef void (*hgraph_data_lifecycle_callback_t)(void* value);
typedef void (*hgraph_data_render_callback_t)(void* value, void* render_ctx);

typedef struct hgraph_data_type_s {
	hgraph_str_t name;
	hgraph_str_t label;
	hgraph_str_t description;

	size_t size;
	size_t alignment;

	hgraph_data_lifecycle_callback_t init;
	// There is intentionally no cleanup callback.
	// Data types must be designed such that:
	//
	// * They are POD and requires no cleanup
	// * They only exists in the pipeline and thus, cleanup can be handled with
	//   pipeline lifecycle.
	//
	// The reason is that in a live reloaded environment, the clean up code is
	// already gone by the time we reload.

	hgraph_io_status_t (*serialize)(const void* value, hgraph_out_t* output);
	hgraph_io_status_t (*deserialize)(void* value, hgraph_in_t* input);
	hgraph_data_render_callback_t render;
} hgraph_data_type_t;

typedef struct hgraph_pin_description_s {
	hgraph_str_t name;
	hgraph_str_t label;
	hgraph_str_t description;

	const hgraph_data_type_t* data_type;
	hgraph_flow_type_t flow_type;
} hgraph_pin_description_t;

typedef struct hgraph_attribute_description_s {
	hgraph_str_t name;
	hgraph_str_t label;
	hgraph_str_t description;

	const hgraph_data_type_t* data_type;

	hgraph_data_lifecycle_callback_t init;
	// There is intentionally no cleanup callback.
	// Attribute just have different defaults from data types and the same
	// reasoning applies.

	hgraph_data_render_callback_t render;
} hgraph_attribute_description_t;

typedef struct hgraph_node_type_s {
	hgraph_str_t name;
	hgraph_str_t label;
	hgraph_str_t description;
	hgraph_str_t group;

	size_t size;
	size_t alignment;

	hgraph_data_lifecycle_callback_t init;
	hgraph_data_lifecycle_callback_t cleanup;

	const hgraph_pin_description_t** input_pins;
	const hgraph_pin_description_t** output_pins;
	const hgraph_attribute_description_t** attributes;

	void (*begin_pipeline)(const hgraph_node_api_t* api);

	void (*execute)(const hgraph_node_api_t* api);

	void (*end_pipeline)(const hgraph_node_api_t* api);

	void (*transfer)(void* dst, void* src);

	void (*render)(
		hgraph_node_part_t part,
		const void* last_status,
		void* render_ctx
	);
} hgraph_node_type_t;

struct hgraph_node_api_s {
	void* (*allocate)(
		const hgraph_node_api_t* api,
		hgraph_lifetime_t lifetime,
		size_t size
	);

	void* (*data)(const hgraph_node_api_t* api);

	void (*report_status)(const hgraph_node_api_t* api, const void* status);

	const void* (*input)(
		const hgraph_node_api_t* api,
		const void* pinOrAttribute
	);
	void (*output)(
		const hgraph_node_api_t* api,
		const hgraph_pin_description_t* pin,
		const void* value
	);
};

struct hgraph_plugin_api_s {
	void (*register_node_type)(
		const hgraph_plugin_api_t* api,
		const hgraph_node_type_t* node_type
	);
};

static inline void*
hgraph_node_allocate(
	const hgraph_node_api_t* api,
	hgraph_lifetime_t lifetime,
	size_t size
) {
	return api->allocate(api, lifetime, size);
}

static inline void*
hgraph_node_data(const hgraph_node_api_t* api) {
	return api->data(api);
}

static inline const void*
hgraph_node_input(const hgraph_node_api_t* api, const void* pinOrAttribute) {
	return api->input != NULL ? api->input(api, pinOrAttribute) : NULL;
}

static inline void
hgraph_node_output(
	const hgraph_node_api_t* api,
	const hgraph_pin_description_t* pin,
	const void* value
) {
	if (api->output != NULL) { api->output(api, pin, value); }
}

static inline void
hgraph_node_report_status(const hgraph_node_api_t* api, const void* status) {
	api->report_status(api, status);
}

static inline void
hgraph_plugin_register_node_type(
	const hgraph_plugin_api_t* api,
	const hgraph_node_type_t* node_type
) {
	api->register_node_type(api, node_type);
}

#endif
