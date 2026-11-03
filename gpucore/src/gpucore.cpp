// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// The core's C interface. Skeleton: each function is a stub until its issue lands
// (docs/code-map.md, gpucore, status column).

#include <cstring>

#include "shadps4_gpucore.h"
#include "shadps4_gpucore_host.h"

namespace {

constexpr int32_t MaxWindowSize = 16384;

bool IsSerialValid(const char* serial) {
    if (serial == nullptr || std::strlen(serial) != 9) {
        return false;
    }
    for (int i = 0; i < 4; ++i) {
        if (serial[i] < 'A' || serial[i] > 'Z') {
            return false;
        }
    }
    for (int i = 4; i < 9; ++i) {
        if (serial[i] < '0' || serial[i] > '9') {
            return false;
        }
    }
    return true;
}

} // namespace

extern "C" {

uint32_t shadps4_gpucore_abi_version(void) {
    return SHADPS4_GPUCORE_ABI_VERSION;
}

const char* shadps4_gpucore_version_string(void) {
    return SHADPS4_GPUCORE_VERSION_STRING;
}

int shadps4_gpucore_check_config(const ShadPs4GpuCoreConfig* config) {
    if (config == nullptr || config->struct_size < sizeof(ShadPs4GpuCoreConfig)) {
        return SHADPS4_GPUCORE_E_INVALID_ARGUMENT;
    }
    if ((config->abi_version >> 16) != SHADPS4_GPUCORE_ABI_MAJOR) {
        return SHADPS4_GPUCORE_E_ABI_MISMATCH;
    }
    if (config->title == nullptr || *config->title == '\0') {
        return SHADPS4_GPUCORE_E_INVALID_ARGUMENT;
    }
    if (!IsSerialValid(config->serial)) {
        return SHADPS4_GPUCORE_E_INVALID_ARGUMENT;
    }
    if (config->user_dir == nullptr || *config->user_dir == '\0') {
        return SHADPS4_GPUCORE_E_INVALID_ARGUMENT;
    }
    if (config->width < 1 || config->width > MaxWindowSize || config->height < 1 ||
        config->height > MaxWindowSize) {
        return SHADPS4_GPUCORE_E_INVALID_ARGUMENT;
    }
    return SHADPS4_GPUCORE_OK;
}

void shadps4_gpucore_register_kernel(void) {}

int shadps4_gpucore_init(const ShadPs4GpuCoreConfig* config, const ShadPs4GpuCoreHost* host) {
    (void)config;
    (void)host;
    return SHADPS4_GPUCORE_E_NOT_BUILT;
}

uintptr_t shadps4_gpucore_resolve(const char* scoped_nid) {
    (void)scoped_nid;
    return 0;
}

int shadps4_gpucore_handle_fault(void* ucontext, void* address) {
    (void)ucontext;
    (void)address;
    return 0;
}

void shadps4_gpucore_shutdown(void) {}

} // extern "C"
