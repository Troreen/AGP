# Refresh painted Landscape weights without re-exporting the mesh

Status: resolved
Blocked by: 04

## Scope

Continue the owner's normal authoring/export workflow. Add an explicit TGA menu
command that refreshes the PNG source for the selected Landscape's prepared
mask, using its material's effective SplatMask and saved world bounds. This is
an incremental authoring step; it must not claim full acceptance or automatically
modify geometry, save assets, reimport textures, convert DDS or deploy content.

## Acceptance

- Exactly one native Landscape must be selected and carry export metadata.
- Resolve the effective mask texture with ticket 04's inheritance rules.
- Validate bounds, loaded Landscape coverage and named Snow_Dunes/Rough/Cliff
  layers before writing. RGB uses that order without normalization.
- Use linear RGBA8 readback, retain source resolution, and publish a complete
  PNG only after successful capture/encoding. Preserve prior output on failure.
- Repaint and refresh can reuse the PNG path without touching the StaticMesh.
- Report that Unreal texture reimport and verified runtime DDS preparation
  remain explicit steps. No authored package is saved by export/tests.
- GPU check with a fresh asymmetric Landscape verifies channel/orientation and
  repaint changes. NullRHI tests alone do not establish mask rendering.

## Verification

Unreal 5.7.4 plugin build passed. Both `TGA.TerrainMask` tests passed on RTX 3070
with offscreen DirectX 11 (`Intermediate/TerrainMaskGpuFirst/index.json`). A new
single-component Landscape produced exact numeric RGB at asymmetric reference
locations; repainting updated green to 90 without changing red/blue or heights.
The same PNG path and original source dimensions were retained. Invalid bounds,
rotation and a missing boundary component preserved the previous file. A locked
Windows PNG forced publication failure after capture and preserved all old bytes.

Independent spec and standards reviews found no confirmed defect. A review of
Unreal's overlap API caught a potential missing-strip check before the first
GPU run: completeness now uses requested grid indices, not the bounding box of
only found components. The final integrated thirteen-check GPU run passed with
zero test warnings/failures (`Intermediate/TerrainContinuationCheckpoint`).
Initial mask creation/import and automatic UV correction remain
separate work; final owner acceptance is still pending.
