# This hack is ugly enough I don't really want it in the main file
# All this file does is get libarchive compiling + linkable as a static target

# Both libarchive + zlib really want you to install them and use the find_package commands, but I
# don't want to do that, I want to build from source, so this takes some fiddling

# Neither project sets a version, so we end up with a warning if we don't specify a policy
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

# zlib sets a min version so old it throws a deprecation warning, disable it
set(CMAKE_WARN_DEPRECATED CACHE BOOL OFF)
add_subdirectory(libs/zlib "${CMAKE_CURRENT_BINARY_DIR}/zlib_build" EXCLUDE_FROM_ALL)
set(CMAKE_WARN_DEPRECATED CACHE BOOL ON)

# Disable extra useless targets
# What sort of name is `example`??
set_target_properties(
    zlib example minigzip
    PROPERTIES
        EXCLUDE_FROM_ALL 1
        EXCLUDE_FROM_DEFAULT_BUILD 1
)

# Make these targets use the default `zconf.h` which gets copied to the build dir
set_property(
    TARGET zlibstatic
    APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${CMAKE_CURRENT_BINARY_DIR}/zlib_build"
)

# Libarchive is horrible about this, it's really not meant to be added as a subdirectory, so it
# fills the cflags with a bunch of garbage
# Back them up so we can restore them after
set(_old_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(_old_CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
set(_old_CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(_old_CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
set(_old_CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
set(_old_CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

# Also turn this off cause it creates a bunch of warnings when run under clang
set(ENABLE_WERROR CACHE BOOL OFF)

# Disable everything from libarchive which tries to find another library
set(ENABLE_BZip2 CACHE BOOL OFF)
set(ENABLE_LZMA CACHE BOOL OFF)
set(ENABLE_LZO CACHE BOOL OFF)
set(ENABLE_LIBB2 CACHE BOOL OFF)
set(ENABLE_LZ4 CACHE BOOL OFF)
set(ENABLE_ZSTD CACHE BOOL OFF)
set(ENABLE_MBEDTLS CACHE BOOL OFF)
set(ENABLE_NETTLE CACHE BOOL OFF)
set(ENABLE_OPENSSL CACHE BOOL OFF)
set(ENABLE_LIBXML2 CACHE BOOL OFF)
set(ENABLE_EXPAT CACHE BOOL OFF)
set(ENABLE_PCREPOSIX CACHE BOOL OFF)
set(ENABLE_CNG CACHE BOOL OFF)

# Disable the extra targets
set(ENABLE_ICONV CACHE BOOL OFF)
set(ENABLE_TEST CACHE BOOL OFF)
set(ENABLE_INSTALL CACHE BOOL OFF)

set(ENABLE_TAR CACHE BOOL OFF)
set(ENABLE_CPIO CACHE BOOL OFF)
set(ENABLE_CAT CACHE BOOL OFF)

# Disable finding zlib, but set the vars as if it worked
set(ENABLE_ZLIB CACHE BOOL OFF)
set(HAVE_LIBZ 1)
set(HAVE_ZLIB_H 1)

add_subdirectory(libs/libarchive EXCLUDE_FROM_ALL)

set_target_properties(
    archive
    PROPERTIES
        EXCLUDE_FROM_ALL 1
        EXCLUDE_FROM_DEFAULT_BUILD 1
)

# Manually add zlib back
set_property(
    TARGET archive_static
    APPEND PROPERTY INCLUDE_DIRECTORIES
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib"
    "${CMAKE_CURRENT_BINARY_DIR}/zlib_build"
)
target_link_libraries(archive_static zlibstatic)

# Setup properties to let us properly link against it
target_compile_definitions(archive_static PUBLIC LIBARCHIVE_STATIC)
set_target_properties(
    archive_static
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/libs/libarchive/libarchive"
)

set(CMAKE_REQUIRED_FLAGS ${_old_CMAKE_REQUIRED_FLAGS})
set(CMAKE_C_FLAGS ${_old_CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS_DEBUG ${_old_CMAKE_C_FLAGS_DEBUG})
set(CMAKE_EXE_LINKER_FLAGS ${_old_CMAKE_EXE_LINKER_FLAGS})
set(CMAKE_SHARED_LINKER_FLAGS ${_old_CMAKE_SHARED_LINKER_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${_old_CMAKE_C_FLAGS_RELEASE})

unset(_old_CMAKE_REQUIRED_FLAGS)
unset(_old_CMAKE_C_FLAGS)
unset(_old_CMAKE_C_FLAGS_DEBUG)
unset(_old_CMAKE_EXE_LINKER_FLAGS)
unset(_old_CMAKE_SHARED_LINKER_FLAGS)
unset(_old_CMAKE_C_FLAGS_RELEASE)
