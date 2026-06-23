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
- Runtime shadow bias tuning controls.
- Demo scene with primitives, a textured floor, a chest mesh, and an animated character.

## Repository Layout

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

The ModelViewer expects to run with `Source/Application/ModelViewer` as the working directory because it resolves `Assets` relative to that path.

## Running ModelViewer

Run `Bin/Debug/ModelViewer.exe` after building, with this working directory:

```text
C:\Users\tarik\Documents\GitHub\AGP\Source\Application\ModelViewer
```

The debug build opens a console window for logs. The `P` key is useful while tuning lights and shadows because it prints copy-paste friendly placement and bias values.

## Controls

### Camera

| Control | Action |
| --- | --- |
| Hold right mouse button | Mouse look |
| `W` / `S` | Move forward / backward |
| `A` / `D` | Move left / right |
| `Space` | Move up |
| `Ctrl` | Move down |

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
| `P` | Log current light placement, active light count, and shadow tuning values |

### Shadow Bias Tuning

Shadow tuning changes are runtime-only. Restarting the application restores the defaults unless the tuned values are copied back into code.

| Control | Action |
| --- | --- |
| `F5` | Reset runtime shadow tuning |
| `F6` / `F7` | Decrease / increase directional shadow bias |
| `F8` / `F9` | Decrease / increase spot shadow bias |
| `F10` / `F11` | Decrease / increase point shadow bias |

## Shadow Mapping Notes

The current shadow setup keeps material textures in low texture slots and binds shadow resources at high slots:

- Directional cascades: `t100` to `t103`
- Spot shadow maps: `t104` to `t107`
- Point cube shadow maps: `t108` to `t111`

Directional shadows use four cascades. Spot shadows use perspective shadow maps matched to the spot light cone. Point shadows use cube depth maps sampled by direction from the light to the shaded world position.
