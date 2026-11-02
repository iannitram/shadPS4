// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// The harness itself: the library links, the headers compile as C, and calls that must stay
// harmless (a null NID, a fault the core does not own, shutdown before init) stay harmless.

#include <gtest/gtest.h>
#include "shadps4_gpucore.h"
#include "shadps4_gpucore_hooks.h"

extern "C" uint32_t shadps4_gpucore_test_c_abi_version(void);
extern "C" size_t shadps4_gpucore_test_c_hooks_v1_size(void);

TEST(GpuCoreSkeleton, HeadersCompileAsC) {
    EXPECT_EQ(shadps4_gpucore_test_c_abi_version(), SHADPS4_GPUCORE_ABI_VERSION);
    EXPECT_EQ(shadps4_gpucore_test_c_hooks_v1_size(), SHADPS4_GPUCORE_HOOKS_V1_SIZE);
}

TEST(GpuCoreSkeleton, NullNidResolvesToZero) {
    EXPECT_EQ(shadps4_gpucore_resolve(nullptr), 0u);
}

TEST(GpuCoreSkeleton, UnknownFaultIsNotHandled) {
    EXPECT_EQ(shadps4_gpucore_handle_fault(nullptr, nullptr), 0);
}

TEST(GpuCoreSkeleton, ShutdownWithoutInitIsSafe) {
    shadps4_gpucore_shutdown();
    SUCCEED();
}
