# Engine Optimisations

This document is the main source of truth for engine optimisation work in the AGP engine. It records what has actually been implemented, why each change exists, the caveats that matter when extending it, and the most sensible future directions.

The current optimisation work focuses on reducing unnecessary render work, separating mutable game state from render state, and preparing the engine for safer threading without changing gameplay, material, shader, or asset behaviour.

## Current Status

Implemented:

- Mesh-local bounding spheres.
- Conservative camera frustum culling for render items.
- Conservative camera frustum filtering for relevant lights.
- Render-scene snapshots that decouple rendering from live `World` and component state.
- Main-thread rendering from immutable snapshots.
- Update-thread ownership of `World`, camera, animation, and demo light mutation in ModelViewer.
- Triple-buffered render snapshot handoff between update and render.
- Copied input frames for update-thread consumption.
- Thread-safe shadow tuning values for F5-F11 debug controls.
- RHI shader-resource/sampler binding scratch buffers without per-call heap allocation.
- Cached global sampler binding list.
- Conservative per-light and per-cascade shadow caster culling.
- Main-thread render-resource prewarm before worker shadow recording.
- Threaded deferred command-list recording for shadow maps with deterministic main-thread playback.
- Reusable header-only snapshot queue and fixed-step update worker utilities.
- Quiet debug instrumentation for render, shadow, snapshot, and fixed-update counters.

Remaining future work:

- A persistent engine-wide job system or thread pool instead of per-frame async shadow jobs.
- Per-face point-light cubemap caster culling.
- Broader integration of the scheduler utilities into applications beyond ModelViewer.
- Frame-time profiling and visual debug overlays.

## Implemented Optimisations

### 1. Mesh Bounding Spheres

Meshes now compute conservative local bounds during `Mesh::Initialize`.

Implementation:

- The mesh scans all vertex positions.
- It builds an axis-aligned min/max range.
- The local sphere center is the midpoint of that range.
- The radius is the farthest vertex distance from that center.
- Meshes without vertices are treated as having invalid bounds.

Reasoning:

- Bounds are needed before any meaningful culling can happen.
- A sphere is cheap to transform and cheap to test against a frustum.
- The sphere is conservative: it may keep extra meshes visible, but it should not incorrectly remove visible meshes.

Caveats:

- This is not a tight optimal bounding sphere.
- Large or oddly shaped meshes may survive culling more often than necessary.
- Invalid or empty bounds are intentionally treated as always visible.

### 2. Camera Frustum Culling

The render snapshot path now builds a simple camera frustum and tests mesh spheres against it.

Implementation:

- The frustum planes are extracted from the camera view-projection matrix so culling matches the projection used by rendering.
- Mesh local bounds are transformed to world space using world position plus max-axis scale.
- Enabled mesh components are collected into two render lists:
  - `ShadowCasters`: all enabled render items.
  - `VisibleRenderItems`: only camera-visible render items.
- The main scene renders only `VisibleRenderItems`.
- Shadow maps still render from `ShadowCasters`.

Reasoning:

- Rendering fewer meshes in the main camera pass is an immediate win.
- Keeping all enabled shadow casters avoids incorrect missing shadows from offscreen objects.
- Conservative culling is better than aggressive culling for a course engine, because visual popping is worse than retaining a few extra draw calls.

Caveats:

- Shadow casters are not culled per light yet.
- Very large bounds can make an object remain visible even when mostly offscreen.
- The implementation assumes world matrix scale can be conservatively represented by max-axis scale.

### 3. Camera-Relevant Light Filtering

The render snapshot also filters lights against the camera frustum.

Implementation:

- Directional lights are always relevant.
- Point and spot lights are included when their radius sphere intersects the camera frustum.
- Only relevant lights are added to the light buffer and considered for shadow map rendering.

Reasoning:

- Offscreen local lights should not consume light-buffer or shadow-map slots when they cannot affect the current camera view.
- Directional lights are global and cannot be culled with a simple radius/frustum test.

Caveats:

- This does not prove that a light affects visible surfaces, only that its influence volume intersects the camera frustum.
- Spot lights use radius only for relevance; cone/frustum intersection is a possible future refinement.
- Offscreen shadow casters are intentionally still used for relevant lights.

### 4. Render Scene Snapshots

Rendering now has a value-style `RenderSceneSnapshot` path.

Implementation:

- `GraphicsEngine::BuildRenderSnapshot(...)` reads the current camera, lights, meshes, materials, transforms, bounds, and animation joint transforms.
- `GraphicsEngine::RenderSnapshot(...)` renders only from the snapshot.
- `GraphicsEngine::Render(...World...)` remains as a compatibility wrapper that builds a temporary snapshot and renders it.

Reasoning:

- Rendering from live `World` and component pointers makes threading dangerous.
- A snapshot gives the render thread a stable view of the frame.
- It is the prerequisite for update/render overlap and later deferred command-list work.

Caveats:

- Snapshot render items still hold shared pointers to mesh and material resources.
- GPU resource creation and lazy material refresh are still render-time concerns.
- The snapshot is not a fully serialized scene; it is a CPU-side render packet for the current engine structure.

### 5. Triple-Buffered Snapshot Handoff

ModelViewer now uses three snapshot buffers to pass render data from the update thread to the main render thread.

Implementation:

- Each buffer has a state:
  - `Free`
  - `Building`
  - `Ready`
  - `Rendering`
- The update thread writes only to a free or old ready buffer.
- Publishing a new snapshot makes older ready snapshots free.
- The render thread acquires the newest ready snapshot.
- If no new snapshot is ready, the render thread reuses the current rendering snapshot instead of blocking.

Reasoning:

- Triple buffering reduces stalls compared with a strict double-buffer swap.
- The main thread can keep rendering while the update thread prepares the next frame.
- Dropping stale ready snapshots is acceptable because rendering the newest state is more useful than rendering every intermediate update snapshot.

Caveats:

- If the render thread holds a snapshot for a long time and the update thread has no free or ready buffer to overwrite, snapshot publication can temporarily skip.
- This is a simple ModelViewer-local pipeline, not a general engine-wide frame graph.
- The current locking is deliberately coarse and simple.

### 6. Fixed-Step Update Worker

ModelViewer now runs simulation/update work on a `std::jthread`.

Implementation:

- The main thread owns:
  - Win32 message pumping.
  - Input event collection.
  - Cursor recentering for mouse look.
  - Command-list recording and execution.
  - Present.
- The update thread owns:
  - `World` mutation.
  - Camera transform updates.
  - Animation input actions.
  - Light toggle/move/aim actions.
  - `World::Update`.
  - Snapshot building and publishing.
- Update uses a fixed 60 Hz step.
- A max catch-up step count prevents long stalls from causing unbounded update loops.

Reasoning:

- The teacher advice was to avoid a separate render thread because rendering should remain on the window-owning thread.
- Moving update work to a worker allows the main thread to render the previous completed snapshot while the next update is prepared.
- Fixed-step update gives stable animation and camera timing.

Caveats:

- This is update/render overlap, not full parallel rendering.
- The main thread must not mutate `World` or components after the update thread starts.
- Debug tooling that reaches directly into live components from the render thread would violate the ownership boundary.
- ModelViewer is the first integration point; this is not yet generalized across all applications.

### 7. Copied Input Frames

Input is copied into a value-type frame before being handed to the update thread.

Implementation:

- The main thread samples key-down and key-pressed states.
- Mouse-look delta is computed on the main thread because cursor position and recentering are Win32/window-thread concerns.
- Pending input frames merge pressed keys and accumulated mouse delta if the update thread has not consumed the previous input yet.
- `FreeFlyCameraController` now has an `InputState` overload for thread-friendly updates.

Reasoning:

- Sharing `InputHandler` across threads would create avoidable races.
- The update thread should consume stable input data, not query live Win32/input state.
- Cursor recentering stays on the thread that owns the window.

Caveats:

- Key presses can be merged across multiple submitted frames before one fixed update consumes them.
- Mouse delta is accumulated while waiting for update consumption.
- This is sufficient for ModelViewer controls, but a real game input system would probably need timestamps or per-frame input events.

### 8. Shadow Tuning Mutex

The shadow bias tuning values are now guarded by a small mutex.

Implementation:

- `AdjustShadowBias`, `ResetShadowTuning`, and `LogShadowTuning` lock the tuning state.
- `GetShadowDepthBias` locks before reading.
- An internal unlocked helper is used when a caller already holds the mutex.

Reasoning:

- F5-F11 tuning controls run on the update thread after the threading change.
- Shadow rendering reads the same bias values on the main render thread.
- The state is tiny, so a simple mutex is clearer than atomics for three related values.

Caveats:

- This only protects shadow tuning values.
- It does not make the whole `GraphicsEngine` thread-safe.
- Rendering APIs, resource creation, command-list execution, and present still belong on the main thread.

### 9. Existing Shadow Quality/Stability Work

The current engine also contains shadow quality/stability improvements that are relevant to optimisation discussions, even though they are mostly visual stability work rather than CPU scheduling work.

Implementation highlights:

- Directional shadows use cascades.
- Cascades have per-cascade depth bias and world-space filter radius data.
- Cascade bounds are padded and texel-snapped for stability.
- Projected directional and spot shadows use Poisson PCF sampling.

Reasoning:

- Stable cascades reduce near-camera clipping, floating, and bias drift.
- World-space filter radius makes directional PCF more consistent between cascades.
- The current point-light cubemap shadow path remains simpler and harder edged.

Caveats:

- These changes improve quality and stability, not necessarily frame time.
- C++ light buffer layout and HLSL light layout must stay matched.
- Future shadow changes should remain narrow and code-evidenced because the shadow path is sensitive.

### 10. RHI Binding Allocation Cleanup

`GraphicsCommandList::SetShaderResources` and `SetShaderSamplers` no longer allocate a heap vector for every bind call.

Implementation:

- Shader-resource binds use a stack `std::array` sized to `D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT`.
- Sampler binds use a stack `std::array` sized to `D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT`.
- Invalid start/count ranges hit `ensure(...)` and return before calling D3D.
- Null resource/sampler entries are still allowed and bind as null slots.

Reasoning:

- These helpers are called frequently while rendering materials, PBR resources, and shadow resources.
- D3D11 slot counts are fixed, so a stack scratch buffer is enough.
- The change removes small repeated heap churn without changing public binding behaviour.

Caveats:

- Invalid ranges still indicate a programming error; the early return only prevents bad D3D calls after the assert.
- The stack buffers are intentionally sized to D3D11 limits, not material limits.

### 11. Cached Global Sampler Bindings

The engine now builds the default sampler binding list once during graphics initialization.

Implementation:

- `mySamplers` reserves space before the three default samplers are created.
- `mySamplerBindings` stores stable pointers to those sampler objects after creation.
- `RenderSnapshot` binds `mySamplerBindings` directly instead of rebuilding a pointer vector each frame.

Reasoning:

- The default sampler set is stable after initialization.
- Rebuilding a vector every frame was unnecessary work.

Caveats:

- If future code adds samplers after initialization, it must rebuild `mySamplerBindings` after any possible `mySamplers` reallocation.

### 12. Per-Light Shadow Caster Culling

Shadow passes now build conservative caster lists per shadow map instead of always drawing every enabled render item.

Implementation:

- Directional shadows cull per cascade against the cascade light-space orthographic bounds.
- Spot shadows cull against the spot light's shadow frustum.
- Point shadows cull by point-light radius only.
- Invalid mesh bounds remain always visible.
- Invalid light/frustum data falls back to keeping casters instead of dropping them.

Reasoning:

- Shadow rendering is draw-call heavy and was still using all enabled render items.
- Per-light and per-cascade culling reduces shadow work while preserving offscreen casters that can still affect the visible scene.
- Point lights use radius-only culling for the first pass because per-face cubemap culling is easier to get visibly wrong.

Caveats:

- Directional and spot culling is conservative but still depends on correct mesh bounds.
- Point shadow passes still render the same radius-filtered caster list for all cubemap faces.
- Missing shadow casters are more visible than extra draw calls, so the implementation intentionally prefers false positives.

### 13. Snapshot Resource Prewarm

`RenderSnapshot` now prepares snapshot render resources before shadow worker jobs are launched.

Implementation:

- The render thread prepares mesh vertex/index buffers for all shadow casters and visible render items.
- Dirty material parameter data is refreshed before worker command-list recording.
- Worker shadow recording uses `RenderMesh(..., false)` so it asserts if lazy resource work would be required.

Reasoning:

- Deferred command-list recording can happen on workers, but lazy mesh/material mutation should not.
- Prewarming keeps worker jobs limited to reading stable snapshot/resource data and recording D3D commands.

Caveats:

- Resource creation still happens in render code; it is just forced onto the main render path before workers.
- A larger engine would likely have explicit resource streaming/preparation instead of render-time prewarm.

### 14. Threaded Shadow Command Lists

Shadow map rendering now records deferred command lists on worker tasks and plays them back on the main thread.

Implementation:

- `RenderSnapshot` builds immutable shadow job descriptions from the current snapshot.
- Each job owns its target shadow map, frame buffer data, override PSO/stages, optional point-shadow buffer, and culled caster list.
- One reusable deferred `GraphicsCommandList` is used per active job.
- Worker tasks record shadow commands into their own command list.
- The main thread waits for workers, then executes completed command lists in deterministic order before the main scene command list is executed.
- Each shadow job unbinds prior shadow SRVs before binding a shadow map as a depth target.

Reasoning:

- D3D11 deferred contexts allow command recording work to be moved off the main thread while immediate-context execution remains on the main/window thread.
- Deterministic playback preserves cascade/light order.
- Keeping `Present` and immediate execution on the main thread follows the course/window ownership constraints.

Caveats:

- This is not a full renderer job system.
- Worker tasks are currently launched per frame/job; a persistent thread pool would reduce scheduling overhead.
- Command-list workers must not read live `World` objects, mutate materials, or create resources lazily.

### 15. Reusable Frame Scheduling Utilities

The ModelViewer scheduling pieces have been moved into a small header-only utility.

Implementation:

- `EngineScheduling::TripleBufferedSnapshotQueue` owns the snapshot states, latest-ready selection, previous-snapshot reuse, and publish/drop counters.
- `EngineScheduling::FixedStepUpdateWorker` owns the fixed-step `std::jthread` loop and tick counter.
- ModelViewer still supplies the app-specific input consumption, world update, snapshot publishing, and idle wait callbacks.

Reasoning:

- The previous triple-buffer and fixed-step loop worked, but the logic was embedded directly in ModelViewer.
- Pulling the reusable mechanics into a utility makes the ownership model easier to reuse without changing gameplay APIs.

Caveats:

- ModelViewer remains the only integrated client.
- This is a small scheduling helper, not a full engine job system.

### 16. Quiet Debug Instrumentation

The engine and ModelViewer now expose lightweight counters through the existing debug logging path.

Implementation:

- `GraphicsEngine::RenderStats` tracks render item counts, visible item counts, light counts, shadow pass counts, caster culling, and shadow command-list recording/execution counts.
- The snapshot queue tracks published, reused, and dropped snapshots.
- The fixed-step worker tracks completed fixed ticks.
- Pressing `P` logs the current light tuning values plus the new runtime counters.

Reasoning:

- Optimisations need quick verification without noisy per-frame logs.
- The existing `P` debug path is already opt-in and useful during manual testing.

Caveats:

- These are counters, not a profiler.
- They do not report CPU/GPU timings yet.

## Render Flow After Optimisation

Current ModelViewer flow:

1. Main thread initializes window, graphics, scene, and command list.
2. Main thread starts the update worker.
3. Main thread pumps Win32 messages.
4. Main thread updates `InputHandler` and publishes a copied input frame.
5. Update thread consumes copied input.
6. Update thread runs fixed-step camera, animation, light, and world updates.
7. Update thread builds a `RenderSceneSnapshot`.
8. Update thread publishes the snapshot into the triple-buffer queue.
9. Main thread acquires the newest ready snapshot.
10. Main thread prewarms mesh/material resources referenced by the snapshot.
11. Main thread builds light-buffer data and shadow job descriptions.
12. Worker tasks record shadow-map deferred command lists from immutable snapshot data.
13. Main thread executes shadow command lists in deterministic order.
14. Main thread records the main scene command list from the immutable snapshot.
15. Main thread executes the main scene command list and presents.

Important ownership rule:

- After `StartUpdateThread`, the update thread owns `World` and component mutation.
- The main thread renders snapshots and must not read or mutate live world/component state.
- Shadow worker tasks may record commands from prepared snapshot/resource data only.

## Verification Performed

Build verification:

```text
AGP.sln /t:ModelViewer /p:Configuration=Debug /p:Platform=x64 /m
```

Result:

```text
Build succeeded.
0 Warning(s)
0 Error(s)
```

Latest checked build:

- Debug x64 `ModelViewer` target, 2026-06-30.
- Built with the sanitized child `cmd` MSBuild invocation because this machine can expose duplicate `PATH`/`Path` environment keys.

Runtime smoke verification:

- Launched `Bin/Debug/ModelViewer.exe` with the ModelViewer project directory as the working directory.
- Waited until newly appended 2026-06-30 log output reached `Ready!`.
- Stopped the process after the smoke test.

What this proves:

- The app starts.
- Graphics initialization reaches completion.
- FBX meshes and animations load.
- The update/render split does not immediately deadlock during startup.

What this does not prove:

- It was not a full interactive visual inspection.
- It did not verify every camera/light hotkey manually.
- It did not inspect D3D debug-layer output for a long interactive session.

## Known Issues And Caveats

### Snapshot Data Is Not Fully Independent

Snapshots copy transforms, light data, camera data, material lists, mesh pointers, and joint transforms. They do not deep-copy mesh or material resource contents.

This is intentional for now, but it means:

- Mesh and material lifetime must remain stable while snapshots can reference them.
- Render-resource preparation still happens in render code.
- Worker-side command-list recording relies on the main-thread prewarm step.

### Shadow Caster Culling Is Conservative

Main-scene objects are camera-frustum culled, and shadow passes now build per-light/per-cascade caster lists.

This reduces draw calls, but it still intentionally keeps uncertain casters:

- Invalid mesh bounds are always visible.
- Directional and spot culling are conservative.
- Point lights use radius-only caster culling, not per-face cubemap culling.
- Shadow passes can still render extra casters to avoid missing visible shadows.

### Light Relevance Is Conservative

Point and spot lights are culled using radius sphere vs camera frustum.

This can keep lights that do not actually affect visible pixels, but it avoids dropping lights incorrectly.

### ModelViewer Is The First Scheduler Client

The reusable snapshot queue and fixed-step worker now live in `Source/Utilities/FrameScheduler.h`, but ModelViewer is still the only integrated client.

This is acceptable for proving the engine path, but broader integration would need:

- Clear rules for other application loops.
- Shared app-loop setup rather than ModelViewer-specific callbacks.

### No Threaded Rendering Yet

Rendering still happens on the main/window thread.

This is intentional. The teacher guidance specifically warned against moving rendering to a separate thread unless the target window is created and owned there.

### Threaded Shadow Command Lists Are Narrow

Shadow map command recording is threaded, but the implementation is intentionally narrow.

Current limits:

- Only shadow-map recording is moved to worker tasks.
- The main thread still executes completed command lists.
- The main thread still records and executes the main scene pass.
- Worker jobs depend on main-thread mesh/material prewarm.
- A persistent thread pool would be more efficient than launching async tasks per frame.

## Future Work

### 1. Persistent Job System Or Thread Pool

Current status:

- Shadow jobs are launched with per-frame async worker tasks.

Recommended change:

- Introduce a small persistent job system or thread pool.
- Reuse worker threads instead of creating/scheduling fresh async work for every shadow job.
- Keep immediate-context execution and `Present` on the main/window thread.

Reasoning:

- Threaded shadow recording now proves the command-list split.
- A persistent worker pool would reduce scheduling overhead and make later jobs easier to add.

### 2. Point-Light Per-Face Caster Culling

Current status:

- Point shadow maps cull casters by light radius only.
- The same filtered caster list is rendered for all six cubemap faces.

Recommended change:

- Add conservative per-face frustum culling for point-light cubemap shadows.
- Keep radius culling as the first broad phase.
- Fall back to the radius list when face data is invalid.

Reasoning:

- Per-face culling can reduce point-shadow draw calls further.
- It should be added only with careful visual verification because face-edge mistakes cause obvious missing shadows.

### 3. Broader Scheduler Integration

Current status:

- The reusable snapshot queue and fixed-step worker exist.
- ModelViewer is still the only application using them.

Recommended change:

- Use the scheduling utilities from other app loops that need update/render overlap.
- Move shared input snapshot patterns into a reusable helper if another app needs them.
- Keep app-specific world/update callbacks outside the utility.

Reasoning:

- The ownership model is useful beyond ModelViewer.
- Shared scheduling code lowers the chance of future app loops violating the snapshot/render boundary.

### 4. Timing Profiling

Current status:

- The engine has counters but no CPU/GPU timing data.

Recommended change:

- Add scoped CPU timers for snapshot build, prewarm, shadow job recording, shadow playback, and main scene recording.
- Add optional GPU timing queries later if the course framework has enough support for them.
- Keep logs opt-in or sampled to avoid frame-by-frame spam.

Reasoning:

- Counters show whether work was skipped.
- Timings show whether the skipped work actually improved frame cost.

### 5. Shadow Debug Visualisation

Current status:

- Culling is conservative and verified by build/smoke startup, but there is no visual debug overlay for caster inclusion.

Recommended change:

- Add temporary/debug-only visualisation for camera frustum, light volumes, and shadow caster inclusion.
- Use it before tightening any culling rule.

Reasoning:

- Missing shadow casters and frustum-edge popping are easier to diagnose visually than from counters alone.

## Maintenance Rules

When changing this optimisation code:

- Keep rendering and `Present` on the window/main thread.
- Do not let the main thread read or mutate live `World` state after the update worker starts.
- Treat snapshots as immutable once published.
- Keep culling conservative unless a debug visualization proves correctness.
- Do not make `GraphicsEngine` broadly multi-threaded by accident.
- Pre-warm resources before recording render commands from worker threads.
- Update this document whenever an optimisation is added, removed, or materially changed.

## Quick File Map

Primary files:

- `Source/Graphics/GraphicsEngine/Objects/Mesh.cpp`
- `Source/Graphics/GraphicsEngine/Objects/Mesh.h`
- `Source/Graphics/GraphicsEngine/GraphicsEngine.cpp`
- `Source/Graphics/GraphicsEngine/GraphicsEngine.h`
- `Source/Graphics/GraphicsEngine/RHI/GraphicsCommandList.cpp`
- `Source/Graphics/GraphicsEngine/RHI/GraphicsCommandList.h`
- `Source/Application/ModelViewer/ModelViewer.cpp`
- `Source/Application/ModelViewer/ModelViewer.h`
- `Source/Utilities/FrameScheduler.h`
- `Source/Utilities/FreeFlyCameraController.cpp`
- `Source/Utilities/FreeFlyCameraController.h`

Important concepts:

- `Mesh::Initialize`: local bounding sphere calculation.
- `GraphicsEngine::BuildRenderSnapshot`: culling and snapshot creation.
- `GraphicsEngine::RenderSnapshot`: resource prewarm, shadow job setup, and rendering from immutable snapshot data.
- `GraphicsCommandList::SetShaderResources` / `SetShaderSamplers`: stack scratch binding helpers.
- `EngineScheduling::TripleBufferedSnapshotQueue`: reusable snapshot handoff.
- `EngineScheduling::FixedStepUpdateWorker`: reusable fixed-step update loop.
- `FreeFlyCameraController::InputState`: thread-friendly camera input.
