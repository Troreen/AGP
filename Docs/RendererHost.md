# Renderer Host Consumer Contract

`RendererHost.h` is AGP's narrow native-window integration boundary. The host owns
the `HWND`, Win32 message loop, application lifetime, and UI. AGP owns its DirectX
11 device, immediate context, swapchain, depth buffer, backbuffer, and presentation.
The contract does not expose AGP `World`, `Actor`, component, or RHI types.

The current contract is `agp-renderer-host/1.1.0`. It supports:

- initializing AGP for one host-owned native window with explicit shader and
  environment-texture paths;
- borrowing the D3D11 device and immediate context for a UI backend such as Dear
  ImGui without transferring ownership;
- resizing AGP's swapchain-backed render targets for non-zero client sizes;
- binding and clearing the AGP-owned backbuffer before host UI draw submission;
- presenting through AGP; and
- stable operation status, code, and message values that a consumer can translate
  into its own diagnostic model.

The returned D3D11 pointers are borrowed and are null unless initialization has
completed successfully and the current frame targets are usable. A consumer must not call `Release`, replace
the immediate context, resize the swapchain directly, or retain the pointers beyond
AGP's process lifetime. Calls are main-thread operations. A minimized host should
skip resize and frame submission while either client dimension is zero.

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
device and swapchain, checks the borrowed interop view, and presents both before
and after resize. It also scans the staged production manifest and
`GraphicsEngine.lib` to reject any test fault-control symbol or legacy environment
activation name.

Deterministic partial-initialization and post-swapchain resize failures use a
separate internal build whose fault seam is compiled out of the production library:

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
