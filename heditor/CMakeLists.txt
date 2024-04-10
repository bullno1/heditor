set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

add_library(heditor_libs SHARED
	./src/libs.c
	../deps/cimgui/cimgui.cpp
	../deps/cimgui/imgui/imgui.cpp
	../deps/cimgui/imgui/imgui_widgets.cpp
	../deps/cimgui/imgui/imgui_draw.cpp
	../deps/cimgui/imgui/imgui_tables.cpp
	../deps/cimgui/imgui/imgui_demo.cpp
)
target_link_libraries(heditor_libs PUBLIC sokol remodule)
target_include_directories(heditor_libs PUBLIC "../deps/cimgui")
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
if (NOT MSVC)
	target_compile_options(heditor_libs PRIVATE -Wno-unused-variable)
endif()

if (MSVC)
	set(HEDITOR_RC ./src/resources.rc)
else()
	set(HEDITOR_RC)
endif()
set(HEDITOR_APP_SOURCES
	./src/app.c
	./src/sfd/sfd.c
	./src/command.c
	./src/detect_debugger.c
	./src/resources.c
	./src/qoi.c
	./src/path.c
	./src/icon.c
	./src/config.c
	./src/allocator/arena.c
	"${HEDITOR_RC}"
)
setup_library(heditor_app FALSE "${HEDITOR_APP_SOURCES}")
target_link_libraries(heditor_app PRIVATE
	heditor_libs
	bullno1-libs
)
target_include_directories(heditor_app PRIVATE "./src")
set_target_properties(heditor_app PROPERTIES PREFIX "")

add_library(heditor_plugin INTERFACE)
target_include_directories(heditor_plugin INTERFACE "include")

add_executable(heditor
	./src/main.c
	./src/allocator/counting.c
)
target_link_libraries(heditor PRIVATE heditor_libs)
