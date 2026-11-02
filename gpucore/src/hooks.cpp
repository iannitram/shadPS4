// SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// The per-game hook table. Skeleton: stubs until the "hook registry" issue lands.

#include "shadps4_gpucore.h"
#include "shadps4_gpucore_hooks.h"

extern "C" {

int shadps4_gpucore_set_hooks(const ShadPs4GpuCoreHooks* hooks) {
    (void)hooks;
    return SHADPS4_GPUCORE_E_NOT_BUILT; // stub: the "hook registry" issue
}

const ShadPs4GpuCoreHooks* shadps4_gpucore_hooks(void) {
    return nullptr; // stub: the "hook registry" issue
}

} // extern "C"
