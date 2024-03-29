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
	HGRAPH_LIFETIME_PIPELINE,
} hgraph_lifetime_t;

typedef enum hgraph_node_part_e {
	HGRAPH_NODE_HEADER,
	HGRAPH_NODE_FOOTER,
} hgraph_node_part_t;

typedef enum hgraph_node_phase_e {
	HGRAPH_NODE_PHASE_BEGIN_EXECUTION,
	HGRAPH_NODE_PHASE_STEP,
	HGRAPH_NODE_PHASE_END_EXECUTION,
} hgraph_node_phase_t;

typedef enum hgraph_flow_type_e {
	HGRAPH_FLOW_ONCE,
	HGRAPH_FLOW_OPTIONAL,
	HGRAPH_FLOW_STREAMING,
	HGRAPH_FLOW_SWITCH,
} hgraph_flow_type_t;

typedef void (*hgraph_node_lifecycle_callback_t)(
	const hgraph_node_api_t* api,
	void* value
);

typedef void (*hgraph_data_initializer_t)(void* value);

typedef struct hgraph_data_type_s {
	hgraph_str_t name;
	hgraph_str_t label;
	hgraph_str_t description;

	size_t size;
	size_t alignment;

	hgraph_data_initializer_t init;

	void (*serialize)(
		const hgraph_node_api_t* api,
		const void* value,
		hgraph_out_t* output
	);
	void (*deserialize)(
		const hgraph_node_api_t* api,
		void* value,
		hgraph_in_t* input
	);

	void (*assign)(void* lhs, const void* rhs);

	void (*render)(
		const hgraph_node_api_t* api,
		const void* value,
		void* render_ctx
	);
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
	hgraph_str_t description;

	const hgraph_data_type_t* data_type;

	hgraph_data_initializer_t init;

	void (*render)(
		const hgraph_node_api_t* api,
		const void* value,
		void* render_ctx
	);
} hgraph_attribute_description_t;

typedef struct hgraph_node_type_s {
	hgraph_str_t name;
	hgraph_str_t label;
	hgraph_str_t description;
	hgraph_str_t group;

	size_t size;
	size_t alignment;
	hgraph_node_lifecycle_callback_t init;
	hgraph_node_lifecycle_callback_t cleanup;

	const hgraph_pin_description_t** input_pins;
	const hgraph_pin_description_t** output_pins;
	const hgraph_attribute_description_t** attributes;

	void (*execute)(const hgraph_node_api_t* api, hgraph_node_phase_t phase);

	void (*render)(
		const hgraph_node_api_t* api,
		hgraph_node_part_t part,
		const void* last_status,
		void* render_ctx
	);
} hgraph_node_type_t;

struct hgraph_node_api_s {
	const void* switch_on;
	const void* switch_off;

	void* (*allocate)(
		const hgraph_node_api_t* api,
		hgraph_lifetime_t lifetime,
		size_t size
	);
	void (*deallocate)(
		const hgraph_node_api_t* api,
		void* ptr
	);

	hgraph_index_t (*id)(const hgraph_node_api_t* api);

	void* (*data)(const hgraph_node_api_t* api);

	void (*status)(const hgraph_node_api_t* api, const void* status);

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

static inline void
hgraph_node_deallocate(
	const hgraph_node_api_t* api,
	void* ptr
) {
	api->deallocate(api, ptr);
}

static inline void*
hgraph_node_data(const hgraph_node_api_t* api) {
	return api->data(api);
}

static inline const void*
hgraph_node_input(const hgraph_node_api_t* api, const void* pinOrAttribute) {
	return api->input(api, pinOrAttribute);
}

static inline void
hgraph_node_output(
	const hgraph_node_api_t* api,
	const hgraph_pin_description_t* pin,
	const void* value
) {
	api->output(api, pin, value);
}

static inline void
hgraph_node_status(const hgraph_node_api_t* api, const void* status) {
	api->status(api, status);
}

static inline void
hgraph_node_switch_on(
	const hgraph_node_api_t* api,
	const hgraph_pin_description_t* pin
) {
	api->output(api, pin, api->switch_on);
}

static inline void
hgraph_node_switch_off(
	const hgraph_node_api_t* api,
	const hgraph_pin_description_t* pin
) {
	api->output(api, pin, api->switch_off);
}

static inline hgraph_index_t
hgraph_node_id(const hgraph_node_api_t* api) {
	return api->id(api);
}

static inline void
hgraph_plugin_register_node_type(
	const hgraph_plugin_api_t* api,
	const hgraph_node_type_t* node_type
) {
	api->register_node_type(api, node_type);
}

#endif
