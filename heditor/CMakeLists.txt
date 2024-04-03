set(SOURCES
	"./main.c"
	"./libs.c"
)
add_executable(heditor "${SOURCES}")
target_link_libraries(heditor PRIVATE sokol)
