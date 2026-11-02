# SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
# SPDX-License-Identifier: GPL-2.0-or-later

# The shared PS4 graphics core (our delta D1, docs/code-map.md). A game consumes it with
#   add_subdirectory(<this fork>/gpucore)   and links   shadps4::gpucore
# It builds only the core, never upstream's emulator app, and leaves upstream's top-level
# CMakeLists.txt untouched. Built on its own (cmake -S gpucore) it also builds its tests.
cmake_minimum_required(VERSION 3.24)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" SHADPS4_GPUCORE_VERSION LIMIT_COUNT 1)
project(shadps4_gpucore VERSION ${SHADPS4_GPUCORE_VERSION} LANGUAGES C CXX)

set(SHADPS4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE INTERNAL "Root of the shadPS4 tree")
option(SHADPS4_GPUCORE_TESTS "Build the core's tests" ${PROJECT_IS_TOP_LEVEL})

add_library(shadps4_gpucore STATIC
    src/gpucore.cpp
    src/hooks.cpp
)
add_library(shadps4::gpucore ALIAS shadps4_gpucore)
target_compile_features(shadps4_gpucore PUBLIC cxx_std_23)
target_include_directories(shadps4_gpucore PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_compile_definitions(shadps4_gpucore PRIVATE
    SHADPS4_GPUCORE_VERSION_STRING="${SHADPS4_GPUCORE_VERSION}")
set_target_properties(shadps4_gpucore PROPERTIES POSITION_INDEPENDENT_CODE ON)

# The upstream sources (src/video_core, src/shader_recompiler, src/common and the Gnm/VideoOut
# libraries under ${SHADPS4_ROOT}) are added by the core-sources issue: docs/code-map.md, gpucore.

if (SHADPS4_GPUCORE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
