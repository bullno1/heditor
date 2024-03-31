set(SOURCES
	"./main.c"

	"./registry.c"
	"./graph.c"

	"./common.c"
	"./plugin1.c"
	"./plugin2.c"
	"./data.c"
)
add_executable(tests "${SOURCES}")
target_link_libraries(tests hgraph_runtime hgraph_plugin)
