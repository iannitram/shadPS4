# SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
# SPDX-License-Identifier: GPL-2.0-or-later

# shadps4-pkg, the standalone package extractor (our delta D6, docs/code-map.md, pkg-extract).
# Builds on its own (cmake -S tools/pkg-extract) without upstream's emulator app.
cmake_minimum_required(VERSION 3.24)
project(shadps4_pkg LANGUAGES CXX)

set(SHADPS4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE INTERNAL "Root of the shadPS4 tree")
option(SHADPS4_PKG_TESTS "Build the extractor's tests" ${PROJECT_IS_TOP_LEVEL})

add_library(shadps4_pkg_cli STATIC cli.cpp)
target_compile_features(shadps4_pkg_cli PUBLIC cxx_std_20)
target_include_directories(shadps4_pkg_cli PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# The key file parser lives beside the package reader, so the emulator's --extract-pkg uses it too.
add_library(shadps4_pkg_keyfile STATIC "${SHADPS4_ROOT}/src/core/file_format/pkg_keyfile.cpp")
target_compile_features(shadps4_pkg_keyfile PUBLIC cxx_std_20)
target_include_directories(shadps4_pkg_keyfile PUBLIC "${SHADPS4_ROOT}/src/core/file_format")

# The package reader (src/core/file_format/pkg*.cpp) and the unpacking loop join here in the
# "standalone extractor" issue.
add_executable(shadps4-pkg main.cpp)
target_link_libraries(shadps4-pkg PRIVATE shadps4_pkg_cli)

if (SHADPS4_PKG_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
