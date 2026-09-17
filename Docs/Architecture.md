# Engine map

For the game-facing API, callback timing and roadmap, see [GameFramework.md](GameFramework.md).

Start with `GraphicsEngine::RenderSnapshot()` for the frame sequence and
`GameApplication::Run()` for the application loop. Paths below are relative to the
repository root.

## Startup and shutdown

`Source/Application/ModelViewer/Main.cpp` enters `GuardedMain()`, creates the
game and passes it to the reusable host. `GameApplication` initialization creates
the window and graphics engine, registers built-in/game components, calls Initialize for bootstrap composition or a scene-ID request, and creates the
scene command list. Graphics initialization creates frame targets, pipeline
states, samplers, constant buffers, shadow maps, and environment resources.
Constant-buffer registration closes before rendering starts.

`Run()` starts the gameplay update worker after initialization. Shutdown stops
and joins that worker before releasing the held snapshot. The host destructor
also stops it so exception unwinding cannot destroy data still in use.

## Thread and snapshot ownership

| Owner | Responsibility | Entry points |
| --- | --- | --- |
| Main/window thread | Window/input handling, GPU resource preparation, scene recording, command playback, presentation | `GameApplication::Run()`, `GraphicsEngine::RenderSnapshot()` |
| Update worker | World, animation, camera and light updates; snapshot construction | `GameApplication::Impl::Advance()`, `BuildAndPublishRenderSnapshot()` |
| Shadow workers | Record independent shadow command lists from prepared resources | `RecordAndExecuteShadows()` |

`Source/Utilities/FrameScheduler.h` supplies the triple buffer queue; GameFramework owns the gameplay worker
and shared fixed/variable phase loop. The renderer holds a snapshot until a newer one is acquired;
the producer cannot overwrite that held buffer. Obsolete ready snapshots can
be dropped. The synchronous update mode uses the same snapshot path.

`GameFrameworkInternal::WorldRenderBridge::Build()` copies camera/light values, world transforms and joint
matrices. Meshes and materials are shared references: their contents must remain
stable while rendering runs. GraphicsEngine receives copied data only; its finalization step performs existing bounds, culling and material routing. A missing camera publishes an empty frame and clears the backbuffer. GPU buffer creation and dirty material refreshes
finish on the main thread before shadow workers start reading them.

Despite its name, `RenderSceneSnapshot::ShadowCasters` stores the complete enabled
and visible mesh collection. Opaque and blended lists index that collection; mixed-material
meshes can appear in both. Shadow jobs borrow pointers into it and remain valid
only while the snapshot is held. Every worker is joined before shadow playback,
serial fallback, or destruction of job data.

## Frame sequence

| Phase | Inputs and output |
| --- | --- |
| Resource preparation | Creates missing mesh buffers and refreshes material data before concurrent reads. |
| `BuildShadowJobs()` | Selects casters and shadow maps; fills the light buffer with matching shadow assignments. |
| `RecordAndExecuteShadows()` | Records one command list per shadow job and plays them back in job order. On failure, joins all launched workers and records all jobs serially into the scene list. |
| `PrepareSceneCommands()` | Clears frame targets, restores scene state, and binds camera constants, samplers, environment and shadow resources. |
| `RenderGBuffer()` | Opaque geometry writes surface data and depth; tangent normals have a separate target used only for that debug view. |
| `RenderAmbientOcclusion()` | Reads GBuffer world positions and normals; writes screen-space AO. |
| `RenderDeferredLighting()` | Reads GBuffer, AO and shadows; accumulates linear light, then composites to the back buffer within the same GPU event. |
| `RenderDebugView()` | Optionally replaces the composite with the selected diagnostic view. |
| `RenderTransparentGeometry()` | Draws blended elements back-to-front using the opaque depth buffer and full light buffer. |

The host finishes and executes the scene command list, then presents. Pass
helpers rely on this order and the shared scene bindings; they are not independent
rendering entry points. Resource unbinding beside each pass prevents read/write
binding conflicts. CPU statistics retain separate preparation, shadow and scene
recording intervals; they do not measure GPU execution time.

## Subsystem locations

| Location | Responsibility |
| --- | --- |
| `Source/Application/ModelViewer` | IGame implementation, scene setup, controls, mesh library, game materials |
| `Source/GameFramework/Public/GameFramework` | Supported gameplay and registration headers |
| `Source/GameFramework/Integration/GameFramework/Integration` | Owned scene input/source and ready asset bindings |
| `Source/GameFramework/Private` | Host, construction, lifecycle, extraction bridge and component implementations |
| `Source/Graphics/GraphicsEngine/GraphicsEngine.cpp` | Frame orchestration, shadow calculations, resource and material creation |
| `Source/Graphics/GraphicsEngine/RHI` | DirectX 11 device/context operations and command lists |
| `Source/Graphics/GraphicsEngine/Objects` | Mesh, texture, buffer and other graphics wrappers |
| `Source/Graphics/GraphicsEngine/Materials` | Material descriptions, parameters, shader compilation support |
| `Source/Graphics/GraphicsEngine/ConstantBuffers` | CPU structures uploaded to shaders |
| `Source/Graphics/GraphicsEngine/Shaders` | Internal passes and material shader code |
| `Source/Utilities` | Scheduling, startup options, camera controls and logging |
| `CommonUtilities/include` | Shared math, input and timer utilities |
| `Tests/EngineOptimisations` | CPU regression coverage for culling, routing and scheduling |

See [EngineOptimisations.md](EngineOptimisations.md) for switches, build/test
commands, scheduling details and the remaining visual acceptance checks.

## Readability conventions

Use `// --- Phase name ---` for major responsibilities in large files and match
GPU event names where available. Describe pass dependencies, ownership and
unusual ordering beside the relevant implementation. Small files need no banners.
Extract cohesive phases while keeping synchronization and fallback contracts
visible together. Avoid helpers whose only purpose is to shorten a few lines.

The root `.clang-format` uses Allman braces, four-column tab indentation and a
140-column limit. `.editorconfig` supplies matching settings for owned C++ files.
Authored source and tests follow these readability rules:

- Always brace control-flow bodies, including single-statement branches and loops.
- Expand function and lambda bodies; keep separate operations on separate lines.
- Use `struct` only for data. Types with constructors, operators or other member
  functions are `class`, with explicit access sections. Preserve public aggregate
  data where callers rely on aggregate initialization.
- Name each lambda capture. Use `[]` when nothing is captured, `[this]` for member
  access, and explicit value/reference captures for local dependencies. Capture
  asynchronous loop indices by value and keep referenced data alive until work joins.

Use clang-format 22 (the readability pass used 22.1.3) on edited C++ files, for example:

```powershell
clang-format -i Source/Graphics/GraphicsEngine/GraphicsEngine.cpp Source/Graphics/GraphicsEngine/GraphicsEngine.h
```

The formatter inserts braces and expands short bodies. Explicit captures and the
class/struct distinction still require code review. With 22.1.3, dry-run can report
blank-line replacements on already formatted CRLF files when SeparateDefinitionBlocks
is enabled; compare formatter output with the file before treating that as drift.

Keep unrelated formatting separate from behavior changes. Vendor libraries,
CommonUtilities, the imported DDS loader, generated resources/shaders and runtime
asset copies are excluded by `.clang-format-ignore`. The authored C++ source and
tests have received the full readability pass; authored HLSL control-flow bodies
also use braces. Runtime shader copies are refreshed by the existing build.
