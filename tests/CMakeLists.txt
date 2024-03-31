set(SOURCES
	"./main.c"

	"./registry.c"
	"./graph.c"
	"./pipeline.c"

	"./common.c"
	"./plugin1.c"
	"./plugin2.c"
	"./data.c"
)
add_executable(tests "${SOURCES}")

set(MATH_LIB "")
include(CheckLibraryExists)
check_library_exists(m expf "" LIBM)
if(LIBM)
	list(APPEND MATH_LIB "m")
endif()

target_link_libraries(tests hgraph_runtime hgraph_plugin ${MATH_LIB})
