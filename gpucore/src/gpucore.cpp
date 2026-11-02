// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// The core's C interface. Skeleton: each function is a stub until its issue lands
// (docs/code-map.md, gpucore, status column).

#include "shadps4_gpucore.h"
#include "shadps4_gpucore_host.h"

extern "C" {

uint32_t shadps4_gpucore_abi_version(void) {
    return 0; // stub: the "ABI version and config check" issue
}

const char* shadps4_gpucore_version_string(void) {
    return ""; // stub: the "ABI version and config check" issue
}

int shadps4_gpucore_check_config(const ShadPs4GpuCoreConfig* config) {
    (void)config;
    return SHADPS4_GPUCORE_E_NOT_BUILT; // stub: the "ABI version and config check" issue
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
