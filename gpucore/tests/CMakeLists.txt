# SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
# SPDX-License-Identifier: GPL-2.0-or-later

include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# Every tests/test_*.cpp joins the one test binary: a new test file needs no CMake change.
file(GLOB SHADPS4_GPUCORE_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")
add_executable(shadps4_gpucore_test abi_c_compile.c ${SHADPS4_GPUCORE_TEST_SOURCES})
target_link_libraries(shadps4_gpucore_test PRIVATE shadps4::gpucore GTest::gtest_main)
# The release the tests expect shadps4_gpucore_version_string() to report.
target_compile_definitions(shadps4_gpucore_test PRIVATE
    SHADPS4_GPUCORE_EXPECTED_VERSION="${SHADPS4_GPUCORE_VERSION}")
gtest_discover_tests(shadps4_gpucore_test)
