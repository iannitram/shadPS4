# Triage of Uncharted-Universal's shadPS4 copy (SPEC M4, issue #17)

- **Vendor base:** shadPS4 `b4e7ae7` (`shadps4-emu/shadPS4`, 2026-10-02).
- **Copy read:** Uncharted-Universal `d7841aed7b9740430881abb03ad99a68158f05c1` (`main`, 2026-11-12), folder `gpu/shadps4/` (maps to `src/`), plus `gpu/third_party/{gcn,sirit,half}` (map to `externals/`).
- **Upstream checked:** `upstream/main` as fetched 2026-11-12 (tip dated 2026-11-10).
- **Counts:** 192 files listed: **140 general**, **22 per-game**, **30 dropped**. Nothing is left unclassified.
- **Marks:** *general* = comes into the core (the note says what it does); *per-game* = moves behind a hook table entry, the callback living in the game repo; *dropped* = upstream main already has it (the note names the commit).
- **Per-game details** are not here (this repo is public); they are in Uncharted-Universal, `docs/shadps4-triage.md`, which also lists the per-game parts inside the "general" files marked below.

## Method

1. Each vendored file `gpu/shadps4/<p>` was compared byte for byte with `src/<p>` at `b4e7ae7`. Of 502 vendored files, 235 are identical and are not listed.
2. 77 more differ only in line endings (the copy uses CRLF in network, np and shadnet files). With line endings ignored they are identical and are not listed. That leaves 190 changed files in `src/` (140 modified, 50 new), plus 2 in `externals/`.
3. `externals/gcn` and `externals/half` are identical to the base. In `externals/sirit` (a submodule upstream) two files differ; they are listed with the commit that brought the same change. The nested SPIRV-Headers copy is not a change.
4. Each of the 190 diffs (and each new file) was read and marked. For every candidate "dropped", the commit was found with `git log upstream/main -S...` or by comparing `git show upstream/main:src/<path>` with the copy. A file is "dropped" only when upstream main has the same behaviour, even if written differently (the note says when the code differs).
5. A file with a mixed diff is marked by its larger part. Where a "general" file also holds a per-game part, the note says so; that part moves behind a hook when the file is ported.

## Files not listed

- **Upstream-only files** (in the vendored folders at `b4e7ae7` but not vendored): `core/libraries/np/np_tus/*` and `np_utility/*`, `core/libraries/videodec/{videodec,videodec_impl,vdecsw}.*`, and `core/libraries/avplayer/avplayer_handle_streamer.*` (14 files). The copy brings its own older `np_tus.*` and `videodec2*` instead. Only the handle streamer was removed on purpose (`VENDOR.txt`: avplayer "without the handle streamer"); it is a build-scope choice and has no mark here. The others are simply not part of the subset and are not "changes".
- **Old-base files:** the network, np and shadnet files came from a shadPS4-based fork on an older base than `b4e7ae7`. Their diffs against the base are mostly code the copy lacks, not edits; where a file has nothing of its own it is marked dropped with the upstream commit that carries that content (and, where it applies, the later upstream commit that replaced it). `VENDOR.txt` says the online client is built only when an online switch is on, and Uncharted-Universal does not turn it on.
- Files with no counterpart under `src/` or `externals/`: none (everything under `gpu/shadps4` maps).

## Files

| file | mark | note |
|---|---|---|
| `externals/sirit/include/sirit/sirit.h` | dropped | c49529087953b38339f7cde9033001dbd478204d shader_recompiler: Improve NaN handling (#5257) (bumps sirit to fc2b42d5e, which adds OpNClamp) |
| `externals/sirit/src/instructions/extension.cpp` | dropped | c49529087953b38339f7cde9033001dbd478204d shader_recompiler: Improve NaN handling (#5257) (bumps sirit to fc2b42d5e, which adds OpNClamp) |
| `src/common/lru_cache.h` | dropped | 3b53747f5 lru_cache: Fix early stop in ForEachItemBelow (#5570) |
| `src/common/ntapi.h` | general | Guard the QUEUE_USER_APC_FLAGS typedef for newer MinGW headers that already define it (build fix). |
| `src/common/path_util.cpp` | general | User directory can be set by an environment variable; no per-user AppData fallback on Windows; drops ZArchive game-image lookup. |
| `src/common/slot_vector.h` | dropped | 0f6d1b870 common: Rework slot vector (#5212) |
| `src/common/thread.cpp` | general | Adds PreciseSleepUntil (high-resolution waitable timer then spin) for the frame limiter; removes guest-thread-name hooks. |
| `src/common/thread.h` | general | Declares PreciseSleepUntil, a microsecond-accurate sleep for frame pacing. |
| `src/core/libraries/avplayer/avplayer.cpp` | general | Catch C++ exceptions in Stop and return an error code so guest code never has to unwind them. |
| `src/core/libraries/avplayer/avplayer_source.cpp` | general | Resolve guest file paths through an embedder-supplied translation callback instead of the built-in mount table. |
| `src/core/libraries/gnmdriver/gnmdriver.cpp` | general | Resets compute queue ring offset on remap; low-latency render hook on flip; optional bounded frames-ahead limit for flip submits. |
| `src/core/libraries/kernel/equeue.cpp` | general | Make the equeue handle table thread-safe (shared_mutex, stable node references) and give equeues their own handle range. |
| `src/core/libraries/network/http.cpp` | per-game | Details in Uncharted-Universal. |
| `src/core/libraries/network/http2.cpp` | dropped | 7fb1a530c Http2 fixes (#4908) |
| `src/core/libraries/network/http2.h` | dropped | 7fb1a530c Http2 fixes (#4908) |
| `src/core/libraries/network/net.cpp` | dropped | 83511183a Net Fixes (#4910) (sceNetGetSockInfo etc.); file since replaced by 2a2f1003b Major net rewrite (#5236) |
| `src/core/libraries/network/net.h` | dropped | 83511183a Net Fixes (#4910) (IP option values, SockInfo types); moved by 2a2f1003b Major net rewrite (#5236) |
| `src/core/libraries/network/net_error.h` | dropped | 83511183a Net Fixes (#4910) (ECANCELED/EIDRM codes); main keeps them in libraries/net/net_error.h via 2a2f1003b (#5236) |
| `src/core/libraries/network/net_upnp.cpp` | general | Support older miniupnpc: call UPNP_GetValidIGD with the pre-API-18 signature when MINIUPNPC_API_VERSION < 18 |
| `src/core/libraries/network/p2p_sockets.cpp` | dropped | 2a2f1003b Major net rewrite (#5236): P2P sockets over a shared UDP transport with vport demux (libraries/net/net_p2p.cpp) |
| `src/core/libraries/network/sockets.h` | dropped | 2a2f1003b Major net rewrite (#5236): P2PDatagram queue and epoll readiness handle (core/net/*) |
| `src/core/libraries/np/np_auth.cpp` | dropped | 7f6b94a10 np_auth rewrite (#5390) (moved to np_auth/np_auth.cpp) |
| `src/core/libraries/np/np_common.cpp` | dropped | 548789cfa Random, mostly pointless fixes (#4959) |
| `src/core/libraries/np/np_handler.cpp` | dropped | 81be94916 spilt np_handler to multiple files based on it purpose. (#5333); content from c9a5a014d, 1921aee3a, d7114f919, b8e60d2da |
| `src/core/libraries/np/np_handler.h` | dropped | 81be94916 spilt np_handler to multiple files based on it purpose. (#5333); declarations from 1921aee3a, d7114f919 |
| `src/core/libraries/np/np_manager.cpp` | dropped | 6de0eed1d Stub sceNpNotifyPlusFeature (#4993) |
| `src/core/libraries/np/np_manager.h` | dropped | 548789cfa Random, mostly pointless fixes (#4959) |
| `src/core/libraries/np/np_matching2/np_matching2.cpp` | per-game | Details in Uncharted-Universal. |
| `src/core/libraries/np/np_matching2/np_matching2_signaling.cpp` | general | Signaling: take a peer endpoint from the UDP packet source first and the self-reported mapped address only as fallback, since NATs can translate ports |
| `src/core/libraries/np/np_trophy.cpp` | dropped | c9a5a014d Trophies online (shadNet) (#4914); localized names in 3e462bf01 fixed displaying of trophy name (#4898) |
| `src/core/libraries/np/np_tus.cpp` | dropped | 1921aee3a sceNpTus support (#4810) and d7114f919 Tss support (#4930); moved to np_tus/np_tus.cpp by 81be94916 (#5333); one local Poll/WaitAsync behaviour edit is not in upstream (dropped with it, re-check after the swap) |
| `src/core/libraries/np/np_tus.h` | dropped | 1921aee3a sceNpTus support (#4810) and d7114f919 Tss support (#4930); lives at np_tus/np_tus.h since 81be94916 (#5333) |
| `src/core/libraries/np/trophy_ui.cpp` | dropped | 3e462bf01 fixed displaying of trophy name (#4898) |
| `src/core/libraries/videodec/video_utils.cpp` | general | CopyNV12Data takes configurable luma height and pitch alignment (defaults unchanged). |
| `src/core/libraries/videodec/video_utils.h` | general | CopyNV12Data signature gains height_alignment and pitch_alignment parameters with unchanged defaults. |
| `src/core/libraries/videodec/videodec2.cpp` | per-game | Details in Uncharted-Universal. |
| `src/core/libraries/videodec/videodec2_impl.cpp` | per-game | Details in Uncharted-Universal. |
| `src/core/libraries/videodec/videodec2_impl.h` | per-game | Details in Uncharted-Universal. |
| `src/core/libraries/videoout/driver.cpp` | general | Frame limiter with precise slots, separate presenter thread, immediate flips, EOP-flip bookkeeping, frame statistics and pacing diagnostics. Also has a per-game part (details in Uncharted-Universal). |
| `src/core/libraries/videoout/driver.h` | general | Declares presenter swap thread, flip request condvar, EOP flip counters and diagnostic accessors. Also has a per-game part (details in Uncharted-Universal). |
| `src/core/libraries/videoout/video_out.cpp` | general | Low-latency submit hook, tolerant out-of-order EOP flip IRQ handling, and flips raised on the recording thread are submitted by a worker. |
| `src/shader_recompiler/backend/spirv/emit_spirv.cpp` | general | Enables physical-storage-buffer addressing for motion-vector shaders; local workgroup X size divided by the workgroup split factor. |
| `src/shader_recompiler/backend/spirv/emit_spirv.h` | general | Declares a diagnostic query for the shader whose image samples are overridden by a debug env var (kept out of the shader cache). |
| `src/shader_recompiler/backend/spirv/emit_spirv_barriers.cpp` | general | Barrier takes a flag that also orders storage-buffer (uniform) memory in the emitted memory semantics. |
| `src/shader_recompiler/backend/spirv/emit_spirv_context_get_set.cpp` | general | Per-lane ReadConst window loads, frag-coord rescale for reduced scene proxies, split-workgroup id mapping, optional NaN guard on float exports. Also has a per-game part (details in Uncharted-Universal). |
| `src/shader_recompiler/backend/spirv/emit_spirv_floating_point.cpp` | dropped | c49529087953b38339f7cde9033001dbd478204d shader_recompiler: Improve NaN handling (#5257) |
| `src/shader_recompiler/backend/spirv/emit_spirv_image.cpp` | general | Debug-only environment override forcing the sampled value of one image in one shader, to bisect visual problems; shader compiled uncached. |
| `src/shader_recompiler/backend/spirv/emit_spirv_instructions.h` | general | EmitBarrier signature takes the instruction so it can read the barrier flags (mechanical follow-up to the barrier flag change). |
| `src/shader_recompiler/backend/spirv/emit_spirv_special.cpp` | general | Object motion-vector output for temporal upscalers: vertex stage stores/loads previous clip position via buffer address, fragment stage writes the screen-space vector. |
| `src/shader_recompiler/backend/spirv/spirv_emit_context.cpp` | general | Motion-vector varyings and output attachment, two extra push-constant members (motion param, scene size) and workgroup-split setup for compute. |
| `src/shader_recompiler/backend/spirv/spirv_emit_context.h` | general | Adds motion-vector ids and predicates, and the compute workgroup split factor, to the emit context. |
| `src/shader_recompiler/frontend/control_flow_graph.cpp` | general | Ends an EXEC-masked divergence scope before a whole-wave scalar ALU op and resumes it afterwards, so waterfall-style loops see the scalar result. |
| `src/shader_recompiler/frontend/decode.h` | general | Adds a position() accessor on the instruction slice, used by the fetch-shader parse cache (only needed if that cache is kept). Not in upstream main; #5112 (already in the base) does not carry it. |
| `src/shader_recompiler/frontend/fetch_shader.cpp` | general | Per-thread parse cache for fetch shaders, and user data read through the snapshot accessor that the info.h change adds (that one-line change must come with info.h). The base already has the #5112 parsing optimisations; whether the cache is still worth keeping is decided at port time. |
| `src/shader_recompiler/frontend/format.cpp` | dropped | 6752d8b19ec5e7f68302e7b5bebedf5447c78eac shader_recompiler: Handle buffer cache invalidation instructions (#5447) |
| `src/shader_recompiler/frontend/translate/scalar_alu.cpp` | dropped | 282e5c792b9f9b4c3328d41785a9b39b5b722eb9 shader_recompiler: Set SCC carry-out on S_ADDC_U32 (#5030); e518a65157b04d0cefec9930e31a86c5685af28e Fix S_SUBB_U32 SCC wrap (#5031) |
| `src/shader_recompiler/frontend/translate/scalar_memory.cpp` | per-game | Details in Uncharted-Universal. |
| `src/shader_recompiler/frontend/translate/translate.cpp` | general | 64-bit output clamp uses 64-bit immediates (upstream main still passes 32-bit bounds); the rest of the file matches upstream fb6367d29 (#5114), c49529087 (#5257) |
| `src/shader_recompiler/frontend/translate/translate.h` | dropped | c49529087953b38339f7cde9033001dbd478204d shader_recompiler: Improve NaN handling (#5257); 1c2b8f97af7d408e27a4d2923c41cdc6e1af920c Implement proper clamping and handling for V_RCP_CLAMP_F32, V_RSQ_CLAMP_F32, V_RSQ_LEGACY_F32, V_LOG_CLAMP_F32 (#5346) |
| `src/shader_recompiler/frontend/translate/vector_alu.cpp` | dropped | c49529087953b38339f7cde9033001dbd478204d shader_recompiler: Improve NaN handling (#5257); also 1c2b8f97af7d (#5346), 94e21778155b (#5185), d1cb90dbbef8 (#5200), dd43d1cac97e (#5194), ef08b1e1fd2a (#5186) |
| `src/shader_recompiler/frontend/translate/vector_interpolation.cpp` | dropped | 2335629296e352b3a52c2c802dccd72810e4037f shader_recompiler: Support P10/P20 V_INTERP_MOV_F32 selectors (#5087) |
| `src/shader_recompiler/frontend/translate/vector_memory.cpp` | general | MUBUF addr64 loads (64-bit address pair in vaddr) are lowered to ReadConst with an env off switch; also maps BUFFER_WBINVL1 to a device memory barrier. |
| `src/shader_recompiler/info.h` | general | Per-draw user-data snapshot accessors for a threaded draw recorder, a null-table-pointer guard in user-data reads, and a compute split-eligible flag. |
| `src/shader_recompiler/ir/debug_print.h` | general | Include path fix ("common/types.h") for the vendored tree layout; mechanical. |
| `src/shader_recompiler/ir/ir_emitter.cpp` | general | IREmitter::Barrier takes a flag recorded on the instruction to order buffer memory too. |
| `src/shader_recompiler/ir/ir_emitter.h` | general | Barrier(bool buffers = false) declaration. |
| `src/shader_recompiler/ir/passes/effect_loop_clamp_pass.cpp` | per-game | Details in Uncharted-Universal. |
| `src/shader_recompiler/ir/passes/flatten_extended_userdata_pass.cpp` | general | SRT walker robustness: locked JIT buffer, fault handler registered once, atomic patching of faulting loads, lane-uniform ReadFirstLane offsets, per-lane ReadConst window. |
| `src/shader_recompiler/ir/passes/interpolated_integer_pass.cpp` | general | On GPUs lacking barycentric-based manual interpolation, nudges truncations of interpolated float varyings so integer values passed as varyings stay exact. |
| `src/shader_recompiler/ir/passes/ir_passes.h` | general | Declares the interpolated-integer pass, the multi-wave LDS barrier query and the (title-gated) effect loop clamp pass. Also has a per-game part (details in Uncharted-Universal). |
| `src/shader_recompiler/ir/passes/resource_patching_pass.cpp` | general | Tracks images that need native size, drops writes to FMask images and ignores thread-id buffer addressing outside compute instead of asserting. |
| `src/shader_recompiler/ir/passes/shader_info_collection_pass.cpp` | general | Marks compute shaders that share nothing between threads as eligible to be run as smaller workgroups. |
| `src/shader_recompiler/ir/passes/shared_memory_barrier_pass.cpp` | general | Inserts LDS and optional buffer barriers so lockstep-wave code is safe on 32-lane subgroups, incl. multi-wave groups with group-uniform condition analysis. Also has a per-game part (details in Uncharted-Universal). |
| `src/shader_recompiler/ir/passes/srt.h` | general | Constants for the ReadConst window flag and size used by the SRT walker and emitter. |
| `src/shader_recompiler/profile.h` | general | Adds profile flags for the integer-interpolation fix and splitting large compute workgroups. |
| `src/shader_recompiler/recompiler.cpp` | general | Optional GCN instruction dump per translated shader (env dir), runs the interpolated-integer pass on affected GPUs. Also has a per-game part (details in Uncharted-Universal). |
| `src/shader_recompiler/resource.h` | general | User-data fetches go through snapshot accessors, image resources gain a needs_native flag, push data gains motion-param and scene-size members. |
| `src/shader_recompiler/runtime_info.h` | general | Motion-vector runtime info, compute indirect flag plus WorkgroupSplit helper, and a DX10_CLAMP property. |
| `src/shadnet/client.cpp` | dropped | c9a5a014d Trophies online (shadNet) (#4914); LookupOnlineId b8e60d2da np_utility initial impelmentation (#4940); version report 83511183a Net Fixes (#4910) |
| `src/shadnet/client.h` | dropped | d7114f919 Tss support (#4930) and c9a5a014d Trophies online (shadNet) (#4914) (TUS/trophy commands, TooLarge, GetNpid) |
| `src/shadnet/shadnet.proto` | dropped | 1921aee3a sceNpTus support (#4810), c9a5a014d Trophies online (shadNet) (#4914), b8e60d2da np_utility initial impelmentation (#4940) |
| `src/video_core/amdgpu/liverpool.cpp` | general | Pipelined command processor: draw-pipe hand-off, deferred fences, register checksums, queue stats, robust packet handling, diagnostics. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/amdgpu/liverpool.h` | general | Adds pending-fence tracking, register dirty state, work-retired flag, processor-thread flag and queue submit timing. |
| `src/video_core/amdgpu/pixel_format.h` | general | Return no number conversion for unknown format combinations instead of asserting. |
| `src/video_core/amdgpu/pm4_cmds.h` | general | Split WaitRegMem Test into a TestValue(value) helper so callers can test a supplied value. |
| `src/video_core/amdgpu/regs.h` | general | Adds RegDirty/RegDelta to record and replay graphics register blocks for draw-preparation workers. |
| `src/video_core/amdgpu/regs_shader.h` | per-game | Details in Uncharted-Universal. |
| `src/video_core/amdgpu/resource.h` | general | Return a safe anisotropy value for undefined sampler fields instead of asserting; mip count clamp is already upstream. |
| `src/video_core/buffer_cache/buffer.cpp` | general | VRAM budget aware buffer allocation with host-memory fallback, concurrent sharing for side queue, and device-address crash diagnostics. |
| `src/video_core/buffer_cache/buffer.h` | general | Tracks device-local bytes per buffer for VRAM accounting. |
| `src/video_core/buffer_cache/buffer_cache.cpp` | general | Reworked GPU/CPU page tracking: side-queue readbacks, GPU-data shadow merging, fault windows, async fences and diagnostics. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/buffer_cache/buffer_cache.h` | general | Declares side-queue download, GPU-data shadowing, stable-binding checks, buffer stats and deferred-copy interfaces. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/buffer_cache/fault_manager.cpp` | general | Optional diagnostic log of GPU page faults. |
| `src/video_core/buffer_cache/memory_tracker.h` | general | Lock-free read-only upload check, write-fault window extension and per-page GPU write frame/tick queries. |
| `src/video_core/buffer_cache/region_definitions.h` | general | Adds guest-frame counter and GPU-write completion tick hook for page tracking. |
| `src/video_core/buffer_cache/region_manager.h` | general | Per-page GPU write frame/tick, write-fault window extension, early-exit tracking and optional hot-page tracking. |
| `src/video_core/cache_storage.cpp` | general | Per-GPU cache directories, atomic temp-file-and-rename blob writes, blob listing, Flush and Clear. |
| `src/video_core/cache_storage.h` | general | Declares per-GPU device tag, blob listing, flush, clear and cache directory accessors. |
| `src/video_core/host_shaders/CMakeLists.txt` | general | Registers the new TAA sharpen compute shader (build change). |
| `src/video_core/host_shaders/buffer_multi_copy.comp` | general | Compute shader that performs many small buffer-to-buffer copies in a single dispatch. |
| `src/video_core/host_shaders/camera_motion.comp` | general | Computes screen-space motion vectors from depth and current/previous camera matrices for temporal upscalers. |
| `src/video_core/host_shaders/depth_resample.frag` | general | Nearest resample of depth/stencil images between native and reduced-size targets where blit is unsupported. |
| `src/video_core/host_shaders/frame_gen_inputs.comp` | general | Produces the scene depth as R32F input for frame generation. |
| `src/video_core/host_shaders/frame_gen_ui.comp` | general | Composites the game's UI over a generated frame by comparing the finished frame to the UI-less picture. |
| `src/video_core/host_shaders/fsr.comp` | general | Clamp RCAS neighbour fetches to the image edge to avoid out-of-bounds texel reads. |
| `src/video_core/host_shaders/taa.comp` | general | Native-resolution temporal anti-aliasing with reprojection, depth rejection and neighbourhood clipping. |
| `src/video_core/host_shaders/taa_sharpen.comp` | general | RCAS sharpening pass applied to the resolved scene with adjustable strength. |
| `src/video_core/host_shaders/upscale_merge.comp` | general | Writes upscaler output back into scene colour while keeping the original alpha, with debug views. |
| `src/video_core/host_shaders/upscale_reactive.comp` | general | Builds an upscaler reactive mask from scene colour before and after blended draws. |
| `src/video_core/landing_gate.h` | per-game | Details in Uncharted-Universal. |
| `src/video_core/page_manager.cpp` | general | Linux write tracking with userfaultfd write-protect (shmem + unpopulated features), signals for read faults, non-fatal ioctl failures. |
| `src/video_core/renderer_vulkan/fsr411/fsr411.cpp` | general | FSR 4.1.1 upscaler wrapper (host-side pass setup and dispatch). |
| `src/video_core/renderer_vulkan/fsr411/fsr411.h` | general | FSR 4.1.1 upscaler wrapper (host-side pass setup and dispatch). |
| `src/video_core/renderer_vulkan/host_passes/fsr_pass.cpp` | general | FSR pass reads a sub-rect of a larger input image; last-output accessor; EASU-only layout barrier fix. |
| `src/video_core/renderer_vulkan/host_passes/fsr_pass.h` | general | FSR pass reads a sub-rect of a larger input image; last-output accessor; EASU-only layout barrier fix. |
| `src/video_core/renderer_vulkan/liverpool_to_vk.cpp` | general | Unknown sampler/format values return safe fallbacks instead of asserting; non-asserting surface format lookup. |
| `src/video_core/renderer_vulkan/liverpool_to_vk.h` | general | Unknown sampler/format values return safe fallbacks instead of asserting; non-asserting surface format lookup. |
| `src/video_core/renderer_vulkan/motion_history.h` | per-game | Details in Uncharted-Universal. |
| `src/video_core/renderer_vulkan/scene_resolution.h` | general | Scene-resolution proxy types shared by the renderer (render-size vs output-size bookkeeping). |
| `src/video_core/renderer_vulkan/ui_composition.h` | per-game | Details in Uncharted-Universal. |
| `src/video_core/renderer_vulkan/vk_bind_helper.h` | general | Bind helper that records descriptor and push-constant updates for the threaded draw path. |
| `src/video_core/renderer_vulkan/vk_camera_motion.cpp` | per-game | Details in Uncharted-Universal. |
| `src/video_core/renderer_vulkan/vk_camera_motion.h` | per-game | Details in Uncharted-Universal. |
| `src/video_core/renderer_vulkan/vk_compute_pipeline.cpp` | general | Fixed subgroup size on Intel; failed compute pipeline creation skips dispatches; stage rebinding support. |
| `src/video_core/renderer_vulkan/vk_compute_pipeline.h` | general | Fixed subgroup size on Intel; failed compute pipeline creation skips dispatches; stage rebinding support. |
| `src/video_core/renderer_vulkan/vk_constant_ring.h` | general | Ring allocator for per-draw constant data used by the threaded draw preparation. |
| `src/video_core/renderer_vulkan/vk_dlss.cpp` | general | Optional DLSS upscaler backend using the NGX Vulkan extensions. |
| `src/video_core/renderer_vulkan/vk_dlss.h` | general | Optional DLSS upscaler backend using the NGX Vulkan extensions. |
| `src/video_core/renderer_vulkan/vk_draw_pipe.h` | general | Two-stage draw pipe: prepare draws on a helper thread, record on the render thread. |
| `src/video_core/renderer_vulkan/vk_draw_prep.cpp` | general | Draw preparation stage split out of the rasterizer for threaded recording. |
| `src/video_core/renderer_vulkan/vk_draw_prep.h` | general | Draw preparation stage split out of the rasterizer for threaded recording. |
| `src/video_core/renderer_vulkan/vk_frame_capture.cpp` | general | Frame capture tool: dumps render targets and textures per frame (DDS/PNG) for debugging. |
| `src/video_core/renderer_vulkan/vk_frame_capture.h` | general | Frame capture tool: dumps render targets and textures per frame (DDS/PNG) for debugging. |
| `src/video_core/renderer_vulkan/vk_frame_generation.cpp` | general | Frame generation pass (interpolated frames from motion vectors and depth) with pacing hooks. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_frame_generation.h` | general | Frame generation pass (interpolated frames from motion vectors and depth) with pacing hooks. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_fsr4.cpp` | general | FSR 4 (v07) upscaler backend. |
| `src/video_core/renderer_vulkan/vk_fsr4.h` | general | FSR 4 (v07) upscaler backend. |
| `src/video_core/renderer_vulkan/vk_gpu_profiler.cpp` | general | GPU profiler with timestamp queries per pass and transfer marks. |
| `src/video_core/renderer_vulkan/vk_gpu_profiler.h` | general | GPU profiler with timestamp queries per pass and transfer marks. |
| `src/video_core/renderer_vulkan/vk_graphics_pipeline.cpp` | general | Retry pipeline creation without the cache on failure, skip draws on error, stage rebinding, motion-vector key field. |
| `src/video_core/renderer_vulkan/vk_graphics_pipeline.h` | general | Retry pipeline creation without the cache on failure, skip draws on error, stage rebinding, motion-vector key field. |
| `src/video_core/renderer_vulkan/vk_instance.cpp` | general | Optional upscaler and low-latency extensions, device fault, side and sparse queues, memory budget tuning, GPU info log, driver deny lists. |
| `src/video_core/renderer_vulkan/vk_instance.h` | general | Optional upscaler and low-latency extensions, device fault, side and sparse queues, memory budget tuning, GPU info log, driver deny lists. |
| `src/video_core/renderer_vulkan/vk_low_latency.cpp` | general | Low-latency integration (Reflex / Anti-Lag 2) with frame markers. |
| `src/video_core/renderer_vulkan/vk_low_latency.h` | general | Low-latency integration (Reflex / Anti-Lag 2) with frame markers. |
| `src/video_core/renderer_vulkan/vk_object_motion.cpp` | per-game | Details in Uncharted-Universal. |
| `src/video_core/renderer_vulkan/vk_object_motion.h` | per-game | Details in Uncharted-Universal. |
| `src/video_core/renderer_vulkan/vk_pipeline_cache.cpp` | general | Threaded pipeline and shader compile with claims, disk driver cache, prepared draws. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_pipeline_cache.h` | general | Threaded pipeline and shader compile with claims, disk driver cache, prepared draws. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_pipeline_common.cpp` | general | Descriptor and push-constant recording deferred through the scheduler; IsValid check. |
| `src/video_core/renderer_vulkan/vk_pipeline_common.h` | general | Descriptor and push-constant recording deferred through the scheduler; IsValid check. |
| `src/video_core/renderer_vulkan/vk_pipeline_serialization.cpp` | general | Parallel pipeline cache warm-up, device-tag invalidation, cache rebuilt instead of closed on mismatch. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_platform.cpp` | general | Address-binding report history for GPU fault diagnosis; upscaler instance extensions. |
| `src/video_core/renderer_vulkan/vk_platform.h` | general | Address-binding report history for GPU fault diagnosis; upscaler instance extensions. |
| `src/video_core/renderer_vulkan/vk_presenter.cpp` | general | Frame generation, frame pacing, PresentImage, low-latency ids, minimized-window skip. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_presenter.h` | general | Frame generation, frame pacing, PresentImage, low-latency ids, minimized-window skip. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_rasterizer.cpp` | general | Threaded draw pipe, memoized state, helper thread and readback infrastructure in the rasterizer. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_rasterizer.h` | general | Threaded draw pipe, memoized state, helper thread and readback infrastructure in the rasterizer. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_runtime.cpp` | general | Profiler transfer marks, image-access hook and access memo in the runtime. |
| `src/video_core/renderer_vulkan/vk_runtime.h` | general | Profiler transfer marks, image-access hook and access memo in the runtime. |
| `src/video_core/renderer_vulkan/vk_scene_resolution.cpp` | general | Scene-resolution proxy images so render targets can run at a different size than the guest expects. |
| `src/video_core/renderer_vulkan/vk_scene_resolution.h` | general | Scene-resolution proxy images so render targets can run at a different size than the guest expects. |
| `src/video_core/renderer_vulkan/vk_scheduler.cpp` | general | Threaded recorder chunks, CommitWith closures, stall and device-lost reports, pending-poll limit. |
| `src/video_core/renderer_vulkan/vk_scheduler.h` | general | Threaded recorder chunks, CommitWith closures, stall and device-lost reports, pending-poll limit. |
| `src/video_core/renderer_vulkan/vk_semaphore.cpp` | general | Device-lost fault report and exit; 5 s stall report on timeline waits. |
| `src/video_core/renderer_vulkan/vk_semaphore.h` | general | Device-lost fault report and exit; 5 s stall report on timeline waits. |
| `src/video_core/renderer_vulkan/vk_shader_hle.cpp` | per-game | Details in Uncharted-Universal. |
| `src/video_core/renderer_vulkan/vk_shader_hle.h` | per-game | Details in Uncharted-Universal. |
| `src/video_core/renderer_vulkan/vk_shader_util.cpp` | per-game | Details in Uncharted-Universal. |
| `src/video_core/renderer_vulkan/vk_staging_buffer_pool.cpp` | general | Longer idle trim, prewarmed blocks, eager trim on small-VRAM cards. |
| `src/video_core/renderer_vulkan/vk_staging_buffer_pool.h` | general | Longer idle trim, prewarmed blocks, eager trim on small-VRAM cards. |
| `src/video_core/renderer_vulkan/vk_swapchain.cpp` | general | Retire old swapchain via oldSwapchain, FifoRelaxed, present-id and low-latency support, extra image for frame generation. |
| `src/video_core/renderer_vulkan/vk_swapchain.h` | general | Retire old swapchain via oldSwapchain, FifoRelaxed, present-id and low-latency support, extra image for frame generation. |
| `src/video_core/renderer_vulkan/vk_temporal_upscaler.cpp` | general | Temporal upscaler host (FSR3.1 path and backend selection) for HDR scene color with reactive mask and UI composite point. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_temporal_upscaler.h` | general | Temporal upscaler host (FSR3.1 path and backend selection) for HDR scene color with reactive mask and UI composite point. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/renderer_vulkan/vk_vram.h` | general | VRAM accounting helpers fed by the memory budget extension. |
| `src/video_core/renderer_vulkan/vk_xess.cpp` | general | Optional XeSS upscaler backend. |
| `src/video_core/renderer_vulkan/vk_xess.h` | general | Optional XeSS upscaler backend. |
| `src/video_core/texture_cache/button_prompts.cpp` | per-game | Details in Uncharted-Universal. |
| `src/video_core/texture_cache/button_prompts.h` | per-game | Details in Uncharted-Universal. |
| `src/video_core/texture_cache/button_prompts_xbox.inc` | per-game | Details in Uncharted-Universal. |
| `src/video_core/texture_cache/image.cpp` | general | VRAM budget-aware image allocation, storage-usage format fallback, last-view lookup cache and image diagnostics. |
| `src/video_core/texture_cache/image.h` | general | Reorders hot Image fields, adds HasGpuValidData, VRAM size tracking and a last-view index. |
| `src/video_core/texture_cache/image_info.cpp` | per-game | Details in Uncharted-Universal. |
| `src/video_core/texture_cache/image_view.cpp` | general | Clamp image view ranges and storage views to the image, use UNORM for storage sRGB, plus T# and view-format diagnostics. |
| `src/video_core/texture_cache/sampler.cpp` | general | Guard null custom border colour table, clamp lod bias and maxLod, accept extra LOD bias for reduced-scale rendering. |
| `src/video_core/texture_cache/sampler.h` | general | Sampler constructor takes an optional extra LOD bias. |
| `src/video_core/texture_cache/texture_cache.cpp` | general | Texture cache with VRAM-aware garbage collection, tiny readbacks, overlap fixes, backing-write invalidation and lookup memoisation. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/texture_cache/texture_cache.h` | general | Declares GC marks, LRU/memo caches, backing-write tracking, tiny readbacks, extra LOD bias samplers and meta-clear counter. Also has a per-game part (details in Uncharted-Universal). |
| `src/video_core/texture_cache/tile_manager.cpp` | general | Record tiling dispatches through the scheduler with copied descriptor infos for threaded recording. |
| `src/video_core/texture_cache/tile_manager.h` | general | Declares RecordTilingDispatch helper. |
| `src/video_core/texture_cache/types.h` | general | Adds SubresourceExtent::CanContain. |
