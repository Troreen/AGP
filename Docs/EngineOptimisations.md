# Deferred renderer optimizations

Selective integration of `engine-optimisations-deferred-rendering` (commits
`2f91e36` and `8c0efbb`) into `Deferred-Rendering`. The forward-only scene path
from that branch is not used. Existing shaders, scene/material/light values,
shadow quality, transparency policy, and F6 views are preserved in source.

## Rendering and snapshots

The pass sequence is shadows, opaque G-buffer, SSAO, deferred lighting,
composite, optional debug view, then forward transparency. Five production
G-buffer targets and the optional tangent-normal target retain their slots.
All four sampler addresses are cached after sampler creation has finished.
Shader resource and sampler binding use bounded stack arrays.

`BuildRenderSnapshot(camera, world, snapshot)` copies camera/light values,
mesh/material references, world transforms, and skinning matrices. Each mesh
instance is stored once in `ShadowCasters`, which is also the complete enabled
mesh collection. Visible opaque and blended lists contain indices into this
collection; mixed-material meshes enter both lists. Shadows use temporary
pointer lists into the held snapshot. No render operation reads live actors.

Opaque indices sort front-to-back and blended indices back-to-front by squared
distance from the camera to the instance origin, with stable ties. Element-level
blend filtering and G-buffer pipeline selection remain in `RenderMesh`.
`Render(commandList, camera, world)` remains a synchronous compatibility wrapper.

## Culling

Static mesh bounding spheres are computed from imported/procedural vertices.
World spheres use maximum-axis scale for orthogonal transforms and a conservative
upper bound on the largest singular value for shear. Invalid/non-finite data
falls back to visibility. Skinned meshes are always retained because bind-pose
bounds do not cover animation.

Camera culling only affects visible lists. Offscreen mesh instances remain
eligible shadow casters. Point/spot influence spheres filter the light list;
directional lights are retained. The same relevant-light list drives deferred
lighting, forward transparency, and shadow assignment. Cascades use light-space
bounds, spots use shadow frusta, and points use influence-radius tests. Existing
light order and shadow limits remain in effect.

## Ownership and scheduling

| Owner | Work |
| --- | --- |
| Main/window thread | Win32 events, copied input, cursor, F6/title, GPU resource preparation, scene recording, playback, Present |
| Update worker | World/camera/animation/light mutation and snapshot building |
| Shadow workers | Independent command recording from held snapshots and prepared resources |

The triple-buffer queue publishes the newest completed snapshot, drops obsolete
ready snapshots, and retains the currently rendered buffer until the consumer
acquires a replacement. A producer cannot overwrite a held snapshot. Fixed
updates run at 60 Hz, clamp elapsed time to 0.25 seconds, and perform at most five
catch-up ticks. Key presses and mouse deltas accumulate until consumed, then clear
after the first fixed tick. Variable Update/LateUpdate callbacks receive their own
frame sample; see [GameFramework.md](GameFramework.md) for the full timing contract. Input is gated on window focus. F6 only cycles render views;
the obsolete shadow-bias hotkeys are not restored.

`GameApplication` explicitly stops/joins its worker before its synchronization,
snapshots, world, or assets are destroyed, including exception unwinding. Worker
exceptions propagate to the main thread; failed snapshot builds cancel their
queue slot. Statistics and shadow-tuning APIs are synchronized.

Mesh and material contents must remain stable after initialization while the
update worker runs. Component resource references may change. Main-thread
preparation creates mesh buffers and refreshes material data, including fallback
materials, before launching shadow workers. Constant-buffer registration closes
after graphics initialization; updates use each command list's own D3D context.

Shadow jobs use one cached deferred context per pass and per-frame `std::async`
workers. Every job completes before deterministic playback on the main thread.
No partial shadow playback occurs if launch, recording, or command-list finishing
fails: all launched jobs are joined, failed contexts are discarded, and all jobs
are recorded serially into the main list. Context creation failure also selects
serial recording. Each job explicitly unbinds shadow SRVs and establishes its
render target and pipeline override.

## Startup comparison switches

Set environment variables to exactly `1` before launching ModelViewer. They are
read during startup and must not be toggled while workers are running.

| Variable | Effect |
| --- | --- |
| `AGP_DISABLE_CULLING` | Retain all enabled meshes/lights in camera and shadow selection |
| `AGP_DISABLE_THREADED_UPDATE` | Run the same fixed-step update on the main thread |
| `AGP_DISABLE_PARALLEL_SHADOWS` | Record identical shadow jobs serially in the main list |

Unset variables enable the optimizations. For example, from PowerShell:

```powershell
$env:AGP_DISABLE_PARALLEL_SHADOWS = '1'
& .\Bin\Release\ModelViewer.exe
Remove-Item Env:AGP_DISABLE_PARALLEL_SHADOWS
```

P prints statistics in both Debug and Release: visible/total meshes, opaque and
blended list sizes, relevant lights, per-pass shadow-caster submissions/culls,
command-list counts, queue publications/reuse/drops, and CPU milliseconds for
snapshot construction, preparation, shadows (including the wait subset), and
scene recording. Caster counts describe mesh submissions, not individual element
draw calls. Timings exclude GPU execution; they are not frame-time measurements.
The worker tick count is zero in synchronous mode.

## Automated validation

Standalone tests require the existing Visual Studio v145 toolchain and Windows
SDK; no testing framework or external package is introduced.

```powershell
msbuild AGP.sln /t:ModelViewer /p:Configuration=Debug /p:Platform=x64 /m
msbuild AGP.sln /t:ModelViewer /p:Configuration=Release /p:Platform=x64 /m
msbuild Tests\EngineOptimisations\EngineOptimisationsTests.vcxproj /p:Configuration=Debug /p:Platform=x64
& .\Bin\Tests\Debug\EngineOptimisationsTests.exe
```

Validated during integration (2026-09-16): Debug and Release x64 builds, CPU tests
for six frustum boundaries, invalid-input fallback, mirrored/nonuniform scale,
sampled shear containment, mixed/opaque/empty routing and stable ordering,
publication/cancellation/reuse/drop behavior, a 10,000-publication concurrent
queue stress case, and worker exception propagation. Existing CommonUtilities
missing-PDB linker warnings remain; the standalone test build also reports an
unused parameter in the existing Camera3D header.

## Remaining acceptance checks and limits

Interactive baseline capture, image comparisons, D3D debug-output inspection,
and Release frame-time measurements have **not** been performed. No visual
equivalence or speedup is claimed from compilation and CPU tests alone.

Before accepting a performance result, compare the switches at identical
resolution, camera, scene state, animation state, and presentation settings.
Check Lit and every F6 view, both transparent chests and their overlap, animated
limbs, offscreen shadow casters, cascade boundaries, and light-radius edges.
Exercise mouse look, focus changes, animation/light controls, repeated shutdown,
and update/render imbalance. Inspect the D3D debug layer for binding hazards,
constant-buffer errors, and command-list failures. Record frame-time distributions
as well as CPU stage times and work counts in a culling-heavy scene.

This integration adds no animation-aware bounds, per-face point-shadow culling,
tiled lighting, light volumes, or persistent thread pool. Per-frame async launch
overhead can outweigh savings for small scenes. The serial switches provide a
comparison path while that is measured.
