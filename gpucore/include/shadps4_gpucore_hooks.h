/* SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

/* Per-game hooks (delta D3): the only way a game changes core behaviour. Per-game fixes live in
 * the game repo and reach the core through this table; general fixes go into the core itself. */
#ifndef SHADPS4_GPUCORE_HOOKS_H
#define SHADPS4_GPUCORE_HOOKS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ShadPs4GpuCoreHooks {
    uint32_t struct_size; /* sizeof(ShadPs4GpuCoreHooks) as the caller was built */
    void* user;           /* passed back to every hook */
    /* A guest shader is about to be compiled. Return 1 and set *out and *out_words to replace
     * its SPIR-V (that memory stays the game's and must live until shutdown), 0 to keep it. */
    int (*patch_shader)(void* user, uint64_t shader_hash, uint32_t stage, const uint32_t* spirv,
                        size_t words, const uint32_t** out, size_t* out_words);
    /* A command buffer is about to be submitted to the GPU queue. */
    void (*before_submit)(void* user, uint32_t queue, uint64_t submit_index);
    /* A frame was presented (the upscaler and frame pacing plug in here). */
    void (*after_present)(void* user, uint64_t frame_index);
} ShadPs4GpuCoreHooks;

/* The struct_size of the first version of the table: a caller's struct_size below this is
 * refused. Fields added later go after after_present and raise SHADPS4_GPUCORE_ABI_MINOR. */
#define SHADPS4_GPUCORE_HOOKS_V1_SIZE \
    (offsetof(ShadPs4GpuCoreHooks, after_present) + sizeof(void (*)(void*, uint64_t)))

/* Copies the table (the caller may free its own copy afterwards). NULL clears every hook.
 * Rules: docs/code-map.md, gpucore, "Hook table". */
int shadps4_gpucore_set_hooks(const ShadPs4GpuCoreHooks* hooks);
/* The core's current copy, or NULL when no hooks are set. */
const ShadPs4GpuCoreHooks* shadps4_gpucore_hooks(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADPS4_GPUCORE_HOOKS_H */
