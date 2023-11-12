cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(dehotfixer VERSION 1.1)
add_library(dehotfixer SHARED)
set_target_properties(dehotfixer PROPERTIES
    EXPORT_COMPILE_COMMANDS True
    COMPILE_WARNING_AS_ERROR True
    PREFIX ""
)
if(MSVC)
    target_compile_options(dehotfixer PRIVATE /W4)
else()
    target_compile_options(dehotfixer PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(libs/minhook)

include(cmake/libarchive_hack.cmake)

add_library(imgui OBJECT
    "libs/imgui/imgui_demo.cpp"
    "libs/imgui/imgui_draw.cpp"
    "libs/imgui/imgui_tables.cpp"
    "libs/imgui/imgui_widgets.cpp"
    "libs/imgui/imgui.cpp"
    "libs/imgui/backends/imgui_impl_dx11.cpp"
    "libs/imgui/backends/imgui_impl_dx12.cpp"
    "libs/imgui/backends/imgui_impl_win32.cpp"
)
target_include_directories(imgui PUBLIC "libs/imgui" "libs/imgui/backends")

add_library(kiero OBJECT "libs/kiero/kiero.cpp")
target_include_directories(kiero PUBLIC "libs/kiero")
target_link_libraries(kiero PUBLIC minhook)

configure_file(versioninfo.rc.in versioninfo.rc)
configure_file(src/version.h.in build_overrides/version.h)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")
target_sources(dehotfixer PRIVATE ${sources} "${PROJECT_BINARY_DIR}/versioninfo.rc")

target_include_directories(dehotfixer PUBLIC "${PROJECT_BINARY_DIR}/build_overrides" "src")
target_link_libraries(dehotfixer PUBLIC
    minhook
    kiero
    imgui
    archive_static

    dxguid.lib
    d3d11.lib
    d3d12.lib
)

target_precompile_headers(dehotfixer PUBLIC "src/pch.h")

install(
    TARGETS
        dehotfixer
    RUNTIME DESTINATION
        .
)
