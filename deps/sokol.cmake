add_library(sokol INTERFACE)
target_include_directories(sokol INTERFACE "sokol")

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
	target_link_libraries(sokol INTERFACE X11 Xi Xcursor GL dl m)
endif()
