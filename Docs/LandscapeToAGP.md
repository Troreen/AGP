# Landscape to AGP: simple step-by-step guide

Work in **AGP-Content**. Keep the editable Landscape in Unreal; AGP receives a
static mesh for the shape and DDS masks describing where you painted.

**Create → sculpt → paint → export Landscape → import static mesh → prepare
mesh → link its Landscape → export mesh and scene → copy → run AGP.**

## 1. Create the Landscape

In Unreal, open your level and switch to **Landscape → Manage → New**.

| Setting | LD's 1×1 starting layout | Previous 2×2 layout |
| --- | --- | --- |
| Section Size | 63×63 quads | 63×63 quads |
| Sections per Component | 1×1 | 1×1 |
| Number of Components | 1×1 | 2×2 |
| Overall Resolution | **64×64 vertices** | **127×127 vertices** |
| X/Y Scale | 100 | 100 |
| Ground size | **63×63 metres** | **126×126 metres** |

Use the 1×1 starting layout if that is LD's choice. It is smaller than the old
Landscape. Set rotation to zero and keep one Landscape actor for this workflow.
Z scale can start at Unreal's default 100 unless the level needs another height
range. Click **Create**. [Epic's size calculation](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-technical-guide-in-unreal-engine?application_version=5.7)

## 2. Sculpt and finish the component layout

Sculpt the terrain into the desired hills, slopes and valleys. Adding components
makes the Landscape larger; deleting components changes its footprint.

**The current normal exporter needs a complete rectangle of components.**
A rectangle can be wider than it is tall. An L-shape, a missing corner, or a hole
from a deleted component needs a pipeline update before this guide can complete.
The exporter reports unsupported layouts instead of guessing at missing terrain.

Starting with one component and adding more does not keep it a one-component
Landscape. Component borders are shared terrain edges. The final import can
combine several components into one static mesh for the graphics artists.

## 3. Assign the landscape material and paint

Create a material instance from **/Game/Materials/MM_Landscape**, save it in
your new level's asset folder, and assign it to the Landscape. Keep the
original master material unchanged. Use these paint layers:

| Layer | Purpose in AGP |
| --- | --- |
| Snow_Dunes | Base ground |
| Snow_Rough | First painted overlay |
| Snow_Cliff | Final painted overlay |

In **Landscape → Paint**, create a Layer Info asset for each target layer in
your new level's own asset folder. Match its type to the original material's
blend settings; do not change the material to accommodate the test. Every paint
target needs a Layer Info asset. The old test maps and their LayerInfo assets
are no longer part of this workflow. [Epic's paint setup](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-paint-mode-in-unreal-engine?application_version=5.7)

Fill Snow_Dunes as the starting ground, then paint the other layers. Paint a
recognizable patch near a landmark so you can check its position in AGP later.
Save the level and its assets.

The current exporter profile expects these exact three snow-layer names and
textures. A different material/layer set needs its export profile updated.

## 4. Export the Landscape's shape

Select **only the native Landscape** and use **File → Export Selected** to save
an authoring FBX. Enable **Force Front X Axis** for this export.

Give it an unmistakable name, such as `MyLandscape_Authoring.fbx`, and keep it
under the Unreal project's authoring folders. This file is the editable source
handoff; the final runtime FBX is exported separately in step 7.

## 5. Import that FBX back into Unreal as a static mesh

Import the authoring FBX into the Content Browser and name the asset, for
example, **SM_MyLandscape**.

Use **Import Normals**, combine the meshes, and bake the imported transforms
into the vertices. Leave material/texture importing and lightmap-UV generation
off for this handoff.

Place one instance in the level. With the tested baked import, start its actor
at location **0,0,0**, rotation **0,0,0**, scale **1,1,1**. Check that it exactly
overlaps the native Landscape; the FBX already contains the terrain's size and
placement.

Keep both actors. Toggle their editor visibility to inspect one surface at a
time. Seeing two overlapping surfaces here is different from having multiple
Landscape components. Both actors must remain included in the TGA scene export.

## 6. Apply the mesh settings that preserve the handoff

Open the static-mesh asset. Under **LOD0 → Build Settings**, enable:

- **Use Full Precision UVs**
- **Use High Precision Tangent Basis**

Keep its imported normals, apply the changes, and save the asset and level.
This preserves the paint UVs and avoids extra precision loss in the next export.

## 7. Prepare textures, then export the final runtime mesh

Run **ExportAssets.bat** from AGP-Content to prepare the surface textures.
The updated plugin exports paint masks directly as DDS; those masks need no
PNG conversion.

Then select **SM_MyLandscape in the Content Browser**, use **Asset Actions →
Export**, and save the final mesh as:

`AGP-Content/Content/Meshes/Landscape/SM_MyLandscape.fbx`

For this **static-mesh asset export**, leave **Force Front X Axis off** and
**Export Source Mesh off**, matching the tested Unreal-only route. These differ
from the initial Landscape export settings in step 4.

Export this final mesh **after** ExportAssets.bat: that batch can overwrite
runtime FBXs with authoring copies. Keep only one runtime FBX with this mesh name
under Content.

This roundtrip fixes the raw Landscape FBX normal problem in AGP's current
importer. The tested route preserves positions and both UV channels without
requiring Blender.

## 8. Link the runtime mesh to the painted Landscape

Select the runtime static-mesh actor in the level and add a **TGA Export Settings**
component. Set **Source Landscape** to your native painted Landscape. Keep
**Do Not Export** off. Leave the surface repeat at **8 metres** and noise strength
at zero for the first comparison.

The source picker explicitly connects shape and paint. The exporter does not
infer this relationship from actor names, mesh UVs or bounding boxes. Ground
floaters opt in through the same component and source picker; use the same
surface/noise settings on actors sharing a source Landscape.

Keep the editable native Landscape in the level. It supplies export data while
AGP renders the static mesh. The exporter applies the ground material to the
linked mesh's exported slots without modifying the Unreal source material.

## 9. Export the level and its paint masks

In the TGA plugin's Landscape export settings, enable **Export Weight (Splat)
Maps** and **Export Material List**. Save your Unreal level, then use
**TGA → Export Level to JSON** and select:

`AGP-Content/Content/ExportedScenes`

The plugin creates a folder for the level containing its JSON, component DDS
weightmaps and a combined runtime atlas for each linked Landscape. Keep these
files together. It writes the surface texture bindings and world-to-mask mapping
into the normal exported material instance. No separate preparation command or
inspection scene is needed.

Components can store the named paint layers in different texture channels. The
exporter reads each component's allocation by layer name. An unallocated layer
on a component contributes zero; it is not a missing-file error. The current
combined atlas uses red=Dunes, green=Rough, blue=Cliff. Weights are linear data
and are not normalized. The source component DDS files retain their own layouts.

If either actor has a TGA Export Settings component, **Do Not Export must be
off**. Merely hiding an actor in the editor is not the same as excluding it.

## 10. Copy the content and run AGP

From AGP-Content, preview and copy into the sibling AGP checkout:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\SyncContent.ps1 -WhatIf
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\SyncContent.ps1 -OverwriteChangedFiles
```

In **AGP/Bin/Settings/ApplicationSettings.json**, set
**current.application.initialScene** to your ordinary exported scene, for example:

`lvl_mylandscape/Lvl_MyLandscape_Level.json`

Build **Game.sln → Debug | x64** using the project's v145 toolset, then fully
restart **Bin/Debug/Game.exe**.

Check the recognizable paint patch, component borders, terrain orientation,
shape and shading. Authored cameras, lighting, transforms and gameplay components
are retained. If no camera is authored, AGP supplies its normal debug camera.
Correct mask placement and matching lighting/material appearance are separate
checks; exact Unreal/AGP surface parity remains a manual acceptance check.

## When the level changes

| Change | Repeat |
| --- | --- |
| Paint only | TGA scene export → content copy → restart AGP |
| Sculpt, add or remove components | Shape export → mesh reimport/settings → final mesh export → TGA export → copy → restart |
| Surface texture files | ExportAssets.bat → restore/re-export final runtime FBX → TGA export → copy → build/restart |

After adding components, check their paint layers too. An irregular final
component layout needs exporter support before the normal handoff works.

For the implementation and verification checkpoint, see
[normal Landscape loading](SplatmapIntegration.md).
