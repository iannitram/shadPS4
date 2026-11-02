/* SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

/* What the game's loader provides to the core (the host seam). In the bbport ports this was
 * the shim/ folder; here it is one table the loader fills, so the core never assumes the
 * emulator's own kernel. Grown by the host-seam issues (docs/code-map.md, gpucore). */
#ifndef SHADPS4_GPUCORE_HOST_H
#define SHADPS4_GPUCORE_HOST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ShadPs4GpuCoreLogLevel {
    SHADPS4_GPUCORE_LOG_DEBUG = 0,
    SHADPS4_GPUCORE_LOG_INFO = 1,
    SHADPS4_GPUCORE_LOG_WARNING = 2,
    SHADPS4_GPUCORE_LOG_ERROR = 3
} ShadPs4GpuCoreLogLevel;

typedef struct ShadPs4GpuCoreHost {
    uint32_t struct_size; /* sizeof(ShadPs4GpuCoreHost) as the caller was built */
    void* user;           /* passed back to every callback */
    /* One log line, UTF-8, no trailing newline. May be NULL (the core then logs to stderr). */
    void (*log)(void* user, int level, const char* message);
    /* Changes the protection of guest memory the core tracks (read, write: 1 or 0). 0 on success. */
    int (*protect)(void* user, uint64_t address, uint64_t size, int read, int write);
    /* Host address of a guest address, or NULL when it is not mapped. */
    void* (*guest_to_host)(void* user, uint64_t address);
} ShadPs4GpuCoreHost;

#ifdef __cplusplus
}
#endif

#endif /* SHADPS4_GPUCORE_HOST_H */
