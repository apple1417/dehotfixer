cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(dehotfixer VERSION 1.0)

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
add_library(dehotfixer SHARED ${sources} "${PROJECT_BINARY_DIR}/versioninfo.rc")

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


set_target_properties(dehotfixer PROPERTIES COMPILE_WARNING_AS_ERROR True)
if(MSVC)
    target_compile_options(dehotfixer PRIVATE /W4)
else()
    target_compile_options(dehotfixer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MSVC)
    # Enable Edit and Continue
    string(REPLACE "/Zi" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    target_compile_options(dehotfixer PUBLIC "$<$<CONFIG:DEBUG>:/ZI>")
    target_link_options(dehotfixer PUBLIC "/INCREMENTAL")

    # UTF-8 encoded source files
    target_compile_options(dehotfixer PUBLIC "/utf-8")
endif()

if (MINGW)
    # Want to link statically into a single file
    target_link_options(dehotfixer PRIVATE "-static")
endif()

# Postbuild
set(POSTBUILD_SCRIPT "postbuild")
if(CMAKE_HOST_WIN32)
    set(POSTBUILD_SCRIPT "${POSTBUILD_SCRIPT}.bat")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${POSTBUILD_SCRIPT}")
    add_custom_command(
        TARGET dehotfixer
        POST_BUILD
        COMMAND ${POSTBUILD_SCRIPT} "\"$<SHELL_PATH:$<TARGET_FILE:dehotfixer>>\""
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
