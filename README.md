# AGP Graphics Programming

C++20 / DirectX 11 graphics programming project for the AGP assignments. The current ModelViewer scene is focused on Assignment 4.2 shadow mapping while preserving the earlier material, texturing, normal map, lighting, primitive mesh, FBX, and animation work.

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
- Demo scene with primitives, a textured floor, a chest mesh, and an animated character.

## Repository Layout

See the [engine map](Docs/Architecture.md) for startup, threading, snapshot
ownership, the frame sequence, and code formatting conventions.

- `AGP.sln` - Visual Studio solution.
- `Source/Application/ModelViewer` - demo application, scene setup, controls, materials, primitive mesh generation.
- `Source/Graphics/GraphicsEngine` - renderer, RHI, shader/material pipeline, shadow rendering.
- `Source/GameFramework` - actors, components, world, camera, lights, mesh components.
- `Source/Utilities` - logging, camera controller, string helpers, common utilities glue.
- `CommonUtilities/include` - math, input, timer, and utility types.
- `Assets` - runtime meshes, animations, textures, and copied shader files.
- `assignment_4_2_shadow_mapping_translation_and_md.md` - assignment notes and translated lecture guidance.

## Build

Open `AGP.sln` in Visual Studio and build the `Debug | x64` configuration.

ModelViewer resolves `Assets` from the executable location; it does not depend on
its working directory. Run `Bin/Debug/ModelViewer.exe` or
`Bin/Release/ModelViewer.exe`. The debug build opens a log console.

The sample installs a C++ scene source in Main.cpp. ModelViewer requests the scene
by ID and registers only its gameplay behaviors. Mesh/material loading stays in
ModelViewerScene.cpp; the engine constructs and starts registered components.
Start with [the gameplay guide](Docs/GameFramework.md), then ModelViewer.cpp and
ModelViewerComponents.cpp. Real Perforce scene integration requires the team inputs
listed in [implementation evidence](Docs/SimplifiedGameFrameworkImplementation.md).
## Controls

### Camera

| Control | Action |
| --- | --- |
| Hold right mouse button | Mouse look |
| `W` / `S` | Move forward / backward |
| `A` / `D` | Move left / right |
| `Space` | Move up |
| `Ctrl` | Move down |

Mouse look keeps the camera upright relative to its actor parent: yaw follows
parent up, while pitch rotates around the camera's turned local right axis and
is limited to ±89°. The resulting local pose composes with any parent transform.

### Animation

| Control | Action |
| --- | --- |
| `Numpad 0` | Play Breathing animation |
| `Numpad 1` | Play Walk animation |
| `Numpad 2` | Play Run animation |
| `Numpad 3` | Play Wave animation, using the partial upper-body layer when available |

### Light Toggles And Placement

The number-row keys `7`, `8`, and `9` also work for the light controls.

| Control | Action |
| --- | --- |
| `7` / `Numpad 7` | Toggle directional light |
| `8` / `Numpad 8` | Toggle point lights |
| `9` / `Numpad 9` | Toggle spot light |
| `Shift + 7` / `Shift + Numpad 7` | Aim the directional light along the current camera direction |
| `Shift + 8` / `Shift + Numpad 8` | Move the first point light to the current camera position |
| `Shift + 9` / `Shift + Numpad 9` | Move the spot light to the camera and aim it along the current camera direction |
| `P` | Log current light placement, active light count, and renderer statistics |

### Scene and diagnostics

| Control | Action |
| --- | --- |
| `R` | Pause/resume chest rotation |
| `F5` | Reload the current scene |
| `F6` | Cycle renderer debug views |
| `F7` | Spawn/destroy the component-offset hierarchy demo |
| `Esc` | Quit |

Renderer comparison switches remain available: `AGP_DISABLE_THREADED_UPDATE`,
`AGP_DISABLE_PARALLEL_SHADOWS`, and the switches documented in
[EngineOptimisations](Docs/EngineOptimisations.md). No performance improvement is
claimed by the framework API changes. Shadow bias setters remain renderer tooling;
there are no sample F5–F11 bias key bindings.
## Shadow Mapping Notes

The current shadow setup keeps material textures in low texture slots and binds shadow resources at high slots:

- Directional cascades: `t100` to `t103`
- Spot shadow maps: `t104` to `t107`
- Point cube shadow maps: `t108` to `t111`

Directional shadows use four cascades. Spot shadows use perspective shadow maps matched to the spot light cone. Point shadows use cube depth maps sampled by direction from the light to the shaded world position.
