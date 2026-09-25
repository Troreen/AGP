# AGP Graphics Programming

C++20 / DirectX 11 graphics programming project for the AGP assignments. The current Game scene is focused on Assignment 4.2 shadow mapping while preserving the earlier material, texturing, normal map, lighting, primitive mesh, FBX, and animation work.

## Features

- DirectX 11 rendering through a small graphics engine/RHI layer.
- Static and skeletal FBX mesh loading.
- Skeletal animation playback with a partial upper-body animation layer.
- Lit and Unlit material paths.
- Albedo and normal map texture support.
- Directional, point, and spot lights.
- Directional cascaded shadow maps.
- Spot light shadow maps.
- Point light cube shadow maps.
- Render-pass inspection and runtime lighting controls.
- Dear ImGui debug UI, toggled with `F9`.
- Demo scene with primitives, a textured floor, a chest mesh, and an animated character.

## Repository Layout

Start with the [game loop and gameplay guide](Docs/GameLoopOnboarding.md)
for frame order and examples of adding game behavior. The
[plain-language engine architecture](Docs/EngineArchitectureBasics.md)
covers gameplay, scenes, assets, and the path to `BeginPlay`. The more detailed
[engine map](Docs/Architecture.md) covers renderer flow, threading, snapshots, and
code formatting conventions.

- `AGP.sln` - Visual Studio solution.
- `Source/Application/Game` - demo application, scene setup, controls, materials, primitive mesh generation.
- `Source/Engine/GraphicsEngine` - renderer, RHI, shader/material pipeline, shadow rendering.
- `Source/Engine/GameFramework` - actors, components, world, camera, lights, mesh components.
- `Source/Utilities` - logging, camera controller, string helpers, common utilities glue.
- `CommonUtilities/include` - math, input, timer, and utility types.
- `Assets` - runtime meshes, animations, textures, and copied shader files.
- `assignment_4_2_shadow_mapping_translation_and_md.md` - assignment notes and translated lecture guidance.

## Build

For a new Windows checkout, run `SetupWindows.bat`. It walks through the FMOD
Studio API 2.02.05 header setup, downloads Premake if needed, and generates
`Game.sln`. Open that solution in Visual Studio 2026 with the Desktop development
with C++ workload and a Windows SDK, then build `Debug | x64`.

Dear ImGui v1.92.4-docking is included in `Dependencies/ImGui`; neither Git nor Perforce
users need a separate ImGui installation. Perforce users should add the new
`Dependencies/ImGui` source and license files, the modified Premake scripts, and
the game/engine source changes to the same changelist. Run `GenerateProject.bat`
after syncing to regenerate `Game.sln` and the Visual Studio projects. Git users
can use the tracked generated projects or regenerate them the same way.

See [ImGui integration](Docs/ImGui.md) for the team workflow. `F9` shows or
hides the debug panel (shown initially in Debug builds). Add game
debug widgets in `Game::DrawDebugUI()` in `Source/Application/Game/Game.cpp`.
That callback runs on the main thread between ImGui's `NewFrame()` and `Render()`.
ImGui is available in all build configurations; Release and Retail start with
the panel hidden. The Win32 input backend consumes mouse and keyboard input
while a debug widget has focus or is hovered.

Game resolves `Content` from the executable location; it does not depend on
its working directory. Run `Bin/Debug/Game.exe` or
`Bin/Release/Game.exe`. The build copies FMOD sound banks into `Audio` beside
`Game.exe`, and the game loads them from there. The debug build opens a log console.

`Main.cpp` constructs the concrete `Game` and `GameApplication`, then calls
`application.Run(game)`. Game selects the initial scene and registers its gameplay
controls. `GameApplication` owns the window, frame loop, current World, and scene
requests. It selects an exported scene file, asks `UnrealSceneImporter` for
`SceneData`, then passes that data to `BuildWorldFromSceneData`.
Start with [the MVP guide](Docs/GameFrameworkMVP.md), then Game.cpp and
the component files in `Source/Application/Game`. Real Perforce scene integration requires the team inputs
listed in [the importer handoff](Docs/ImporterHandoff.md).

## Controls

### Camera

| Control | Action |
| --- | --- |
| Hold right mouse button | Mouse look |
| `W` / `S` | Move forward / backward |
| `A` / `D` | Move left / right |
| `Space` | Move up |
| `Ctrl` | Move down |

Mouse look keeps the camera upright relative to world up: yaw follows
world up, while pitch rotates around the camera's turned local right axis and
is limited to ±89°. Actor hierarchies are deferred in the MVP.

### Scene and diagnostics

| Control | Action |
| --- | --- |
| `Escape` | Quit the game |
| `F1` | Toggle debug camera |
| `F4` | Reload the current scene |
| `F5` | Select the previous renderer debug view |
| `F6` | Select the next renderer debug view |
| `F7` | Spawn/destroy an extra chest |
| `F8` | Attach a smaller, self-spinning child chest that orbits the extra chest |

Gameplay now runs synchronously. Renderer comparison switches include
`AGP_DISABLE_PARALLEL_SHADOWS` and the switches documented in
[EngineOptimisations](Docs/EngineOptimisations.md). No performance improvement is
claimed by the framework API changes. Shadow bias setters remain renderer tooling;
there are no sample F5–F11 bias key bindings.
## Shadow Mapping Notes

The current shadow setup keeps material textures in low texture slots and binds shadow resources at high slots:

- Directional cascades: `t100` to `t103`
- Spot shadow maps: `t104` to `t107`
- Point cube shadow maps: `t108` to `t111`

Directional shadows use four cascades. Spot shadows use perspective shadow maps matched to the spot light cone. Point shadows use cube depth maps sampled by direction from the light to the shaded world position.
