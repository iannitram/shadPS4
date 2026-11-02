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
file(GLOB SHADPS4_PKG_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")
add_executable(shadps4_pkg_test ${SHADPS4_PKG_TEST_SOURCES})
target_link_libraries(shadps4_pkg_test PRIVATE shadps4_pkg_cli shadps4_pkg_keyfile GTest::gtest_main)
gtest_discover_tests(shadps4_pkg_test)

# End-to-end runs of the shadps4-pkg binary join here in the "standalone extractor" issue.
