set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

add_library(heditor_libs SHARED "./libs.c")
target_link_libraries(heditor_libs PUBLIC sokol remodule)
target_compile_definitions(heditor_libs PUBLIC
	SOKOL_DLL
	REMODULE_SHARED
	PICO_LOG_SHARED
)
target_compile_definitions(heditor_libs PRIVATE
	SOKOL_IMPL
	REMODULE_HOST_IMPLEMENTATION
	REMODULE_MONITOR_IMPLEMENTATION
	PICO_LOG_IMPLEMENTATION
)
set_target_properties(heditor_libs PROPERTIES PREFIX "")

add_library(heditor_app SHARED "./app.c")
target_link_libraries(heditor_app PRIVATE heditor_libs)
set_target_properties(heditor_app PROPERTIES PREFIX "")

add_executable(heditor "./main.c")
target_link_libraries(heditor PRIVATE heditor_libs)
