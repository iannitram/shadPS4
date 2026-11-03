// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Locked tests: ABI version and config check (docs/code-map.md, gpucore).

#include <cstring>
#include <string>
#include <gtest/gtest.h>
#include "shadps4_gpucore.h"

namespace {

ShadPs4GpuCoreConfig Valid() {
    ShadPs4GpuCoreConfig c{};
    c.struct_size = sizeof(ShadPs4GpuCoreConfig);
    c.abi_version = SHADPS4_GPUCORE_ABI_VERSION;
    c.title = "Test Title";
    c.serial = "ABCD12345";
    c.user_dir = "/tmp/user";
    c.sdk_version = 0x05008001;
    c.psf_attributes = 0;
    c.width = 1280;
    c.height = 720;
    return c;
}

// Checks a copy so the call cannot change the caller's struct unseen.
int Check(const ShadPs4GpuCoreConfig& c) {
    const ShadPs4GpuCoreConfig frozen = c;
    const int result = shadps4_gpucore_check_config(&frozen);
    EXPECT_EQ(std::memcmp(&frozen, &c, sizeof(c)), 0) << "check_config changed its input";
    return result;
}

} // namespace

TEST(GpuCoreAbi, VersionIsMajorShiftedSixteenOrMinor) {
    EXPECT_EQ(shadps4_gpucore_abi_version(), SHADPS4_GPUCORE_ABI_VERSION);
    EXPECT_EQ(SHADPS4_GPUCORE_ABI_VERSION,
              (uint32_t(SHADPS4_GPUCORE_ABI_MAJOR) << 16) | uint32_t(SHADPS4_GPUCORE_ABI_MINOR));
}

TEST(GpuCoreAbi, VersionStringIsTheVersionFile) {
    const char* v = shadps4_gpucore_version_string();
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(std::string(v), std::string(SHADPS4_GPUCORE_EXPECTED_VERSION));
}

TEST(GpuCoreConfig, ValidConfigIsOk) {
    EXPECT_EQ(Check(Valid()), SHADPS4_GPUCORE_OK);
}

TEST(GpuCoreConfig, NullIsInvalid) {
    EXPECT_EQ(shadps4_gpucore_check_config(nullptr), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
}

TEST(GpuCoreConfig, SmallStructSizeIsInvalid) {
    auto c = Valid();
    c.struct_size = sizeof(ShadPs4GpuCoreConfig) - 1;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
    c.struct_size = 0;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
}

TEST(GpuCoreConfig, LargerStructSizeIsOk) {
    auto c = Valid();
    c.struct_size = sizeof(ShadPs4GpuCoreConfig) + 16;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_OK);
}

TEST(GpuCoreConfig, OtherMajorIsAbiMismatch) {
    auto c = Valid();
    c.abi_version = (uint32_t(SHADPS4_GPUCORE_ABI_MAJOR + 1) << 16) | SHADPS4_GPUCORE_ABI_MINOR;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_ABI_MISMATCH);
}

TEST(GpuCoreConfig, OtherMinorIsNotCompared) {
    auto c = Valid();
    c.abi_version = (uint32_t(SHADPS4_GPUCORE_ABI_MAJOR) << 16) | 0xFFFFu;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_OK);
}

TEST(GpuCoreConfig, StructSizeIsCheckedBeforeAbi) {
    auto c = Valid();
    c.struct_size = 4;
    c.abi_version = 0xFFFF0000u;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
}

TEST(GpuCoreConfig, AbiIsCheckedBeforeFields) {
    auto c = Valid();
    c.abi_version = 0xFFFF0000u;
    c.title = nullptr;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_ABI_MISMATCH);
}

TEST(GpuCoreConfig, TitleMustBeSet) {
    auto c = Valid();
    c.title = nullptr;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
    c.title = "";
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
}

TEST(GpuCoreConfig, SerialIsFourCapitalsThenFiveDigits) {
    auto c = Valid();
    for (const char* bad : {"", "ABCD1234", "ABCD123456", "abcd12345", "ABC123456", "ABCDE1234",
                            "ABCD1234X", "AB-D12345", "ABCD 2345"}) {
        c.serial = bad;
        EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT) << "serial \"" << bad << "\"";
    }
    c.serial = nullptr;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
    c.serial = "ZZZZ00000";
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_OK);
}

TEST(GpuCoreConfig, UserDirMustBeSet) {
    auto c = Valid();
    c.user_dir = nullptr;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
    c.user_dir = "";
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT);
}

TEST(GpuCoreConfig, SizeIsOneTo16384) {
    auto c = Valid();
    for (int32_t bad : {0, -1, 16385, -16384}) {
        c = Valid();
        c.width = bad;
        EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT) << "width " << bad;
        c = Valid();
        c.height = bad;
        EXPECT_EQ(Check(c), SHADPS4_GPUCORE_E_INVALID_ARGUMENT) << "height " << bad;
    }
    c = Valid();
    c.width = 1;
    c.height = 16384;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_OK);
}

TEST(GpuCoreConfig, SdkVersionAndAttributesAreNotChecked) {
    auto c = Valid();
    c.sdk_version = 0;
    c.psf_attributes = 0xFFFFFFFFu;
    EXPECT_EQ(Check(c), SHADPS4_GPUCORE_OK);
}
