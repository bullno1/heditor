set(SOURCES
	"src/registry.c"
	"src/graph.c"
	"src/migration.c"
	"src/io.c"
	"src/ptr_table.c"
	"src/slot_map.c"
	"src/slip.c"
)
setup_library(hgraph_runtime FALSE "${SOURCES}")
target_include_directories(hgraph_runtime PUBLIC "include")

add_library(hgraph_plugin INTERFACE)
target_include_directories(hgraph_plugin INTERFACE "include")
