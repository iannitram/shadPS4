// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// The per-game hook table: the core keeps its own copy of the table the game sets.

#include <algorithm>
#include <cstring>

#include "shadps4_gpucore.h"
#include "shadps4_gpucore_hooks.h"

namespace {

ShadPs4GpuCoreHooks g_hooks{};
bool g_hooks_set = false;

} // namespace

extern "C" {

int shadps4_gpucore_set_hooks(const ShadPs4GpuCoreHooks* hooks) {
    if (hooks == nullptr) {
        g_hooks = ShadPs4GpuCoreHooks{};
        g_hooks_set = false;
        return SHADPS4_GPUCORE_OK;
    }
    if (hooks->struct_size < SHADPS4_GPUCORE_HOOKS_V1_SIZE) {
        return SHADPS4_GPUCORE_E_INVALID_ARGUMENT;
    }
    ShadPs4GpuCoreHooks copy{};
    std::memcpy(&copy, hooks, std::min<size_t>(hooks->struct_size, sizeof(ShadPs4GpuCoreHooks)));
    copy.struct_size = sizeof(ShadPs4GpuCoreHooks);
    g_hooks = copy;
    g_hooks_set = true;
    return SHADPS4_GPUCORE_OK;
}

const ShadPs4GpuCoreHooks* shadps4_gpucore_hooks(void) {
    return g_hooks_set ? &g_hooks : nullptr;
}

} // extern "C"