/* SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

/* Compiled as C: the public headers must stay plain C for the games' loaders. */
#include "shadps4_gpucore.h"
#include "shadps4_gpucore_hooks.h"
#include "shadps4_gpucore_host.h"

uint32_t shadps4_gpucore_test_c_abi_version(void) {
    return SHADPS4_GPUCORE_ABI_VERSION;
}

size_t shadps4_gpucore_test_c_hooks_v1_size(void) {
    return SHADPS4_GPUCORE_HOOKS_V1_SIZE;
}
