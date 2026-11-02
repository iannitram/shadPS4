/* SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

/* C interface of the shared PS4 graphics core (our fork's delta D1/D2, docs/code-map.md).
 * A game's native loader links shadps4::gpucore and talks to it only through this header,
 * shadps4_gpucore_host.h (what the loader provides) and shadps4_gpucore_hooks.h (per-game hooks).
 * Every struct starts with struct_size so a newer library can read an older caller's struct. */
#ifndef SHADPS4_GPUCORE_H
#define SHADPS4_GPUCORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A change that breaks a caller raises MAJOR; an addition raises MINOR (SPEC.md M2). */
#define SHADPS4_GPUCORE_ABI_MAJOR 0
#define SHADPS4_GPUCORE_ABI_MINOR 1
#define SHADPS4_GPUCORE_ABI_VERSION \
    (((uint32_t)(SHADPS4_GPUCORE_ABI_MAJOR) << 16) | (uint32_t)(SHADPS4_GPUCORE_ABI_MINOR))

typedef enum ShadPs4GpuCoreResult {
    SHADPS4_GPUCORE_OK = 0,
    SHADPS4_GPUCORE_E_INVALID_ARGUMENT = -1,
    SHADPS4_GPUCORE_E_ABI_MISMATCH = -2,
    SHADPS4_GPUCORE_E_NOT_BUILT = -3, /* the skeleton: this part is not written yet */
    SHADPS4_GPUCORE_E_ALREADY_INITIALIZED = -4
} ShadPs4GpuCoreResult;

/* What the game passes at start (the generalized BbGpuConfig of the bbport ports). */
typedef struct ShadPs4GpuCoreConfig {
    uint32_t struct_size;    /* sizeof(ShadPs4GpuCoreConfig) as the caller was built */
    uint32_t abi_version;    /* SHADPS4_GPUCORE_ABI_VERSION as the caller was built */
    const char* title;       /* window title, UTF-8, not empty */
    const char* serial;      /* title id: 4 uppercase letters + 5 digits; names the pipeline cache */
    const char* user_dir;    /* pipeline cache and logs folder, UTF-8, not empty */
    uint32_t sdk_version;    /* from the eboot's procparam */
    uint32_t psf_attributes; /* param.sfo ATTRIBUTE */
    int32_t width;           /* initial window size in pixels, 1..16384 */
    int32_t height;          /* 1..16384 */
} ShadPs4GpuCoreConfig;

struct ShadPs4GpuCoreHost; /* shadps4_gpucore_host.h */

/* The ABI version this library was built as (SHADPS4_GPUCORE_ABI_VERSION). */
uint32_t shadps4_gpucore_abi_version(void);
/* The core release this library was built from: the text of gpucore/VERSION, e.g. "0.1.0". */
const char* shadps4_gpucore_version_string(void);
/* SHADPS4_GPUCORE_OK when config is usable, else the reason (rules: docs/code-map.md, gpucore). */
int shadps4_gpucore_check_config(const ShadPs4GpuCoreConfig* config);

/* Registers the kernel event queues (needed with or without graphics). */
void shadps4_gpucore_register_kernel(void);
/* Creates the window, Vulkan device, presenter and GPU command processor. */
int shadps4_gpucore_init(const ShadPs4GpuCoreConfig* config,
                         const struct ShadPs4GpuCoreHost* host);
/* Function for an imported NID ("NID#lib#mod"), or 0 when the core does not provide it. */
uintptr_t shadps4_gpucore_resolve(const char* scoped_nid);
/* Called first by the loader's fault handler: 1 when a GPU page-tracking fault was handled. */
int shadps4_gpucore_handle_fault(void* ucontext, void* address);
/* Stops the GPU threads and frees the device. Safe to call when init failed or never ran. */
void shadps4_gpucore_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADPS4_GPUCORE_H */
