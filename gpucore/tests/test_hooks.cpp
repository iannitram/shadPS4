// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Locked tests: the hook registry (docs/code-map.md, gpucore, "Hook table").

#include <cstring>
#include <memory>
#include <gtest/gtest.h>
#include "shadps4_gpucore.h"
#include "shadps4_gpucore_hooks.h"

namespace {

void Submit(void*, uint32_t, uint64_t) {}
void Present(void*, uint64_t) {}

ShadPs4GpuCoreHooks Table() {
    ShadPs4GpuCoreHooks h{};
    h.struct_size = sizeof(ShadPs4GpuCoreHooks);
    h.user = reinterpret_cast<void*>(0x1234);
    h.before_submit = &Submit;
    h.after_present = &Present;
    return h;
}

class Hooks : public ::testing::Test {
protected:
    void SetUp() override {
        shadps4_gpucore_set_hooks(nullptr);
    }
    void TearDown() override {
        shadps4_gpucore_set_hooks(nullptr);
    }
};

} // namespace

TEST_F(Hooks, NoneSetAtStart) {
    EXPECT_EQ(shadps4_gpucore_hooks(), nullptr);
}

TEST_F(Hooks, NullClearsAndIsOk) {
    const auto t = Table();
    ASSERT_EQ(shadps4_gpucore_set_hooks(&t), SHADPS4_GPUCORE_OK);
    EXPECT_EQ(shadps4_gpucore_set_hooks(nullptr), SHADPS4_GPUCORE_OK);
    EXPECT_EQ(shadps4_gpucore_hooks(), nullptr);
}

TEST_F(Hooks, SetCopiesTheTable) {
    auto caller = std::make_unique<ShadPs4GpuCoreHooks>(Table());
    ASSERT_EQ(shadps4_gpucore_set_hooks(caller.get()), SHADPS4_GPUCORE_OK);
    const ShadPs4GpuCoreHooks* got = shadps4_gpucore_hooks();
    ASSERT_NE(got, nullptr);
    EXPECT_NE(got, caller.get()) << "hooks() must return the core's copy";
    std::memset(caller.get(), 0xAB, sizeof(*caller)); // the caller reuses its memory
    caller.reset();
    got = shadps4_gpucore_hooks();
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->user, reinterpret_cast<void*>(0x1234));
    EXPECT_EQ(got->before_submit, &Submit);
    EXPECT_EQ(got->after_present, &Present);
    EXPECT_EQ(got->patch_shader, nullptr);
    EXPECT_EQ(got->struct_size, sizeof(ShadPs4GpuCoreHooks));
}

TEST_F(Hooks, SetDoesNotChangeTheCallersTable) {
    const auto t = Table();
    const auto frozen = t;
    ASSERT_EQ(shadps4_gpucore_set_hooks(&t), SHADPS4_GPUCORE_OK);
    EXPECT_EQ(std::memcmp(&t, &frozen, sizeof(t)), 0);
}

TEST_F(Hooks, SmallStructSizeIsRefusedAndKeepsTheOldTable) {
    const auto first = Table();
    ASSERT_EQ(shadps4_gpucore_set_hooks(&first), SHADPS4_GPUCORE_OK);
    auto small = Table();
    small.user = nullptr;
    small.struct_size = static_cast<uint32_t>(SHADPS4_GPUCORE_HOOKS_V1_SIZE - 1);
    EXPECT_EQ(shadps4_gpucore_set_hooks(&small), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
    const ShadPs4GpuCoreHooks* got = shadps4_gpucore_hooks();
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->user, reinterpret_cast<void*>(0x1234));
}

TEST_F(Hooks, SmallStructSizeWithNoTableSetLeavesNone) {
    auto small = Table();
    small.struct_size = 0;
    EXPECT_EQ(shadps4_gpucore_set_hooks(&small), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
    EXPECT_EQ(shadps4_gpucore_hooks(), nullptr);
}

TEST_F(Hooks, ExactlyV1SizeIsAccepted) {
    auto t = Table();
    t.struct_size = static_cast<uint32_t>(SHADPS4_GPUCORE_HOOKS_V1_SIZE);
    ASSERT_EQ(shadps4_gpucore_set_hooks(&t), SHADPS4_GPUCORE_OK);
    const ShadPs4GpuCoreHooks* got = shadps4_gpucore_hooks();
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->after_present, &Present);
    EXPECT_EQ(got->struct_size, sizeof(ShadPs4GpuCoreHooks));
}

TEST_F(Hooks, NewerCallersLargerTableReadsOnlyKnownFields) {
    struct Newer {
        ShadPs4GpuCoreHooks known;
        void (*added_later)(void*, uint64_t);
    };
    Newer n{};
    n.known = Table();
    n.known.struct_size = sizeof(Newer);
    n.added_later = &Present;
    ASSERT_EQ(shadps4_gpucore_set_hooks(&n.known), SHADPS4_GPUCORE_OK);
    const ShadPs4GpuCoreHooks* got = shadps4_gpucore_hooks();
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->before_submit, &Submit);
    EXPECT_EQ(got->struct_size, sizeof(ShadPs4GpuCoreHooks));
}

TEST_F(Hooks, TableWithNoCallbacksIsStillSet) {
    ShadPs4GpuCoreHooks empty{};
    empty.struct_size = sizeof(ShadPs4GpuCoreHooks);
    ASSERT_EQ(shadps4_gpucore_set_hooks(&empty), SHADPS4_GPUCORE_OK);
    const ShadPs4GpuCoreHooks* got = shadps4_gpucore_hooks();
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->before_submit, nullptr);
    EXPECT_EQ(got->after_present, nullptr);
    EXPECT_EQ(got->patch_shader, nullptr);
}

TEST_F(Hooks, SecondSetReplacesTheFirst) {
    auto a = Table();
    ASSERT_EQ(shadps4_gpucore_set_hooks(&a), SHADPS4_GPUCORE_OK);
    auto b = Table();
    b.user = reinterpret_cast<void*>(0x5678);
    b.before_submit = nullptr;
    ASSERT_EQ(shadps4_gpucore_set_hooks(&b), SHADPS4_GPUCORE_OK);
    const ShadPs4GpuCoreHooks* got = shadps4_gpucore_hooks();
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->user, reinterpret_cast<void*>(0x5678));
    EXPECT_EQ(got->before_submit, nullptr);
}
