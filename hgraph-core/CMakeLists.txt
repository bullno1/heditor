set(SOURCES
	./src/types.c
	./src/nodes/in.c
	./src/nodes/out.c
	./src/reg.c
)
setup_library(hgraph_core FALSE "${SOURCES}")
target_include_directories(hgraph_core PUBLIC "include")
target_link_libraries(hgraph_core PUBLIC
	hgraph_plugin
	heditor_plugin
	remodule
)
