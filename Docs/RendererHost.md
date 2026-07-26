# Renderer Host Consumer Contract

`RendererHost.h` is AGP's narrow native-window integration boundary. The host owns
the `HWND`, Win32 message loop, application lifetime, and UI. AGP owns its DirectX
11 device, immediate context, swapchain, depth buffer, backbuffer, and presentation.
The contract does not expose AGP `World`, `Actor`, component, or RHI types.

The current contract is `agp-renderer-host/1.3.0`. It supports:

- initializing AGP for one host-owned native window with explicit shader and
  environment-texture paths;
- borrowing the D3D11 device and immediate context for a UI backend such as Dear
  ImGui without transferring ownership;
- resizing AGP's swapchain-backed render targets for non-zero client sizes;
- binding and clearing the AGP-owned backbuffer before host UI draw submission;
- creating renderer-owned static-mesh and `surface_lit_opaque` material resources
  from caller-owned value arrays and explicit DDS paths;
- submitting an immutable value snapshot with perspective camera, mesh/material
  handles, positive transforms, shadow flags, and one directional preview light;
- reporting renderer scene counts without exposing AGP `World`, `Actor`, component,
  `Mesh`, `Material`, command-list, or RHI types;
- presenting through AGP; and
- stable operation status, code, and message values that a consumer can translate
  into its own diagnostic model.

The returned D3D11 pointers are borrowed and are null unless initialization has
completed successfully and the current frame targets are usable. A consumer must not call `Release`, replace
the immediate context, resize the swapchain directly, or retain the pointers beyond
AGP's process lifetime. Calls are main-thread operations. A minimized host should
skip resize and frame submission while either client dimension is zero.

Resource creation copies CPU mesh values and creates AGP-owned resources. Returned
64-bit handles are opaque, process-local, and invalid after
`ReleaseRendererResource`. A snapshot borrows its item range only for the duration
of `RenderRendererSceneSnapshot`; AGP converts it immediately and retains no pointer
to caller storage. Resource handles must remain live through submission. Material
creation accepts only the versioned `surface_lit_opaque` preset and requires exact
caller-provided albedo, tangent-space normal, and packed AO/roughness/metalness DDS
paths. Renderer-host material creation decodes all three exact textures without
fallback and registers the material handle only after every texture succeeds;
decode failures name the slot/path and leave the output handle invalid. AGP's
existing internal material creation path retains its default-texture fallback for
ModelViewer and legacy engine content. Each scene item applies its one material
handle to every mesh submesh, which
matches the V1 static-mesh renderer component. The host never searches project
content or interprets editor asset IDs.

Static-mesh inputs follow the same structural boundary as AGP's versioned artifact:
every vertex float must be finite, global and per-submesh index counts must form
triangle lists, ranges must be non-empty and in bounds, and every index in a
submesh must reference that submesh's declared vertex range. Rotations use explicit
`RendererEulerDegrees { Yaw, Pitch, Roll }` values to preserve AGP's transform
convention at the staged ABI boundary. Material validation diagnostics name the
exact texture slot and caller path. Scene resource diagnostics name the item index
and both opaque handles; result storage remains fixed-size and value-owned.

Scene submission occurs after `BeginRendererHostFrame` and before UI submission and
`PresentRendererHostFrame`. AGP records and executes its own renderer command list,
then releases command-list references so a later host resize can proceed. Empty
snapshots remain valid and render the explicit preview camera/light with no scene
items. AGP prepares every mesh GPU buffer before recording draws. If any preparation
fails, scene submission returns `renderer.scene_resource_preparation_failed`, no
scene command list is executed, and statistics are not published for the rejected
snapshot.

Initialization is single-attempt once AGP starts creating D3D11 resources. A
failure before that point (invalid arguments or missing/invalid input paths) may be
corrected and retried. A failure after resource creation starts returns
`renderer.restart_required`, hides native views, and requires a new process.

Resize first creates candidate depth resources. Failures before swapchain mutation
preserve the previous targets. If a failure occurs after swapchain mutation, AGP
returns `renderer.resize_recovery_required` and rejects begin/present until another
non-zero resize succeeds. A failed present similarly requires process restart.
Result code and message text are stored in fixed-size value-owned buffers, so they
remain valid without sharing allocation ownership across the static-library ABI.

## Stage the bundle

From the AGP repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\StageAGPRendererHost.ps1 -Configuration Debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\StageAGPRendererHost.ps1 -Configuration Release
```

The generated, ignored consumer contract is
`Artifacts/AGPRendererHost/<Configuration>/x64/`:

```text
AGPRendererHostBundle.json
include/AGP/RendererHost.h
lib/GraphicsEngine.lib
lib/GameFramework.lib
lib/Logger.lib
lib/CommonUtilities.lib
shaders/...
fixtures/T_Shipyard.dds
fixtures/T_Chest_C.dds
fixtures/T_Chest_N.dds
fixtures/T_Chest_M.dds
```

`GameFramework.lib` is an internal static-link dependency of AGP's current combined
graphics object file. Its headers are deliberately absent: consumers neither create
nor edit game-framework objects through this contract. `fixtures/T_Shipyard.dds` is
used only by the bundle verification executable; product hosts pass their own
preview environment resource.

The manifest records configuration, platform, host version, required Windows system
libraries, file roles, lengths, and SHA-256 hashes. Consumers use only this staged
bundle and must not search the AGP checkout as a fallback.

## Verify the staged consumer

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\TestAGPRendererHostBundle.ps1 -Configuration Debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\TestAGPRendererHostBundle.ps1 -Configuration Release
```

The test compiles solely against the staged header and libraries, creates a hidden
Win32 window, checks representative path diagnostics, initializes a real D3D11
device and swapchain, checks the borrowed interop view, creates a mesh and generic
lit material solely through the public staged header, submits an immutable
snapshot, verifies scene statistics, releases its resources, and presents both
before and after resize. A temporary truncated DDS proves the exact material path
rejects corrupt texture data without returning a handle or leaving test artifacts.
The test also scans the staged production manifest and
`GraphicsEngine.lib` to reject any test fault-control symbol or legacy environment
activation name.

Deterministic partial-initialization, post-swapchain resize, and scene mesh-buffer
preparation failures use a separate internal build whose fault seam is compiled out
of the production library:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\TestAGPRendererHostFaults.ps1 -Configuration Debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\TestAGPRendererHostFaults.ps1 -Configuration Release
```

That script builds `GraphicsEngineFaultInjection.lib` in an isolated ignored test
artifact directory with `AGP_RENDERER_HOST_TEST_FAULTS`, links only the internal
fault-test executable against it, and runs each process-terminal lifecycle case in
a fresh process. Neither the internal header nor this library is staged for
consumers. Production staging explicitly forces `RendererHostFaultInjection=false`
before compiling `GraphicsEngine.lib`.
