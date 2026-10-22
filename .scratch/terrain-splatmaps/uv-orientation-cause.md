# Why the reimported Landscape mesh has different UV orientation

Latest owner clarification, 2026-10-22: **yaw -90 is intentional and must remain**.
Rotation does not corrupt UVs; it turns geometry while UVs stay attached. Only
the separate V flip is unwanted. The earlier recommendation below to remove yaw
is superseded. An approved scratch-FBX test now removes the V flip while keeping
yaw -90, with identical geometry/topology and corrected UVs at every corner:
[procedure and result](asset-correction-procedure.md),
[numeric evidence](audit-uv-corrected-yaw-minus90.json).
Production assets remain unchanged. World-space mapping and final appearance
remain separate later checks.

Investigated 2026-10-22, read-only. No reimport, FBX modification, material
change, engine change or new rendering experiment was performed.

## Result

Two separate operations explain the observed difference:

1. The saved Interchange import pipeline applies **Yaw = -90 degrees** to the
   imported geometry. That rotates the terrain relative to world axes while
   retaining its vertex UV values.
2. Unreal's native Landscape FBX exporter writes `(U,V)` directly, but its
   Interchange FBX importer converts incoming UVs to `(U,1-V)`. The ordinary
   StaticMesh exporter already applies that compensating V conversion;
   the checked Landscape export path does not.

These together reproduce the current mesh's recorded UV-versus-position
formula. The quarter turn is not an unavoidable FBX conversion: it is an
explicit persisted import offset. The remaining V inversion comes from the
checked Landscape-export/import convention mismatch. Nothing here requires a
shader workaround to explain or repair the authored mesh pipeline.

## Observations and exact derivation

The live mesh audit checked 4,096 vertices (first vertex instance for each) and
found the following UV0 formula with residual around 1e-14:

```text
current U = (2800 - meshLocalY) / 100
current V = 1 - (meshLocalX + 2800) / 100
```

It also found UV1 carrying the same orientation with a 5700 divisor. This is
direct imported-mesh data rather than appearance inferred from a screenshot.
Source: [live UV audit](audit-live-mesh-uv.json).

The native Landscape vertex factory instead supplies grid coordinates as UV0,
including component section base. For the recorded Landscape transform
translation(-2800,-2800), XY scale100, rotation0:

```text
native U = (nativeWorldX + 2800) / 100
native V = (nativeWorldY + 2800) / 100
```

Sources:
[LandscapeVertexFactory.ush:365](<C:/Program Files/Epic Games/UE_5.7/Engine/Shaders/Private/LandscapeVertexFactory.ush:365>),
[LandscapeRender.cpp:2297](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/LandscapeRender.cpp:2297>),
[recorded scene transform](unreal-live-investigation.md).

The saved import pipeline explicitly serializes
`ImportOffsetRotation=(Pitch=0,Yaw=-90,Roll=0)`.
The pipeline source turns this field into a quaternion in
`ImportOffsetTransform` and stores it as the global import offset. The StaticMesh
factory reads that offset and incorporates it into the transform used for
baked scene-node meshes. Sources:
[saved pipeline:92](audit-import-pipeline-assets.t3d:92),
[InterchangeGenericAssetsPipeline.cpp:1287](<C:/Program Files/Epic Games/UE_5.7/Engine/Plugins/Interchange/Runtime/Source/Pipelines/Private/InterchangeGenericAssetsPipeline.cpp:1287>),
[InterchangeStaticMeshFactory.cpp:97](<C:/Program Files/Epic Games/UE_5.7/Engine/Plugins/Interchange/Runtime/Source/Import/Private/Mesh/InterchangeStaticMeshFactory.cpp:97>),
[baked node transform:157](<C:/Program Files/Epic Games/UE_5.7/Engine/Plugins/Interchange/Runtime/Source/Import/Private/Mesh/InterchangeStaticMeshFactory.cpp:157>).

Let the exported grid coordinate be `(u,v)`. Before that extra yaw, the geometry
corresponds to `(100u-2800,100v-2800)`. Applying yaw-90 gives:

```text
meshLocalX = 100v - 2800
meshLocalY = 2800 - 100u
```

The importer retains `U=u` and supplies `V=1-v`. Substituting the two position
equations reproduces the measured formula exactly. This is a causal
source-and-data match, without claiming a fresh reimport experiment.

The asset investigation additionally found only one raw FBX model, directly
under root, with translation(-2800,+2800,0), scale100, no model/pivot/pre/post/
geometric rotation overrides, and zero rotation template defaults. Thus the
file's model transform does not require an additional hidden quarter turn to
explain the imported asset. Source: [asset/geometry investigation](assets-geometry-research.md)
and its raw FBX inspection record. The relevant FBX global axis convention is
UpZ+, FrontY-, CoordX+, unit1; matching ordinary exporter/importer source
defaults do not independently require a -90 import offset.

## Why the V inversion survives a normal export/import

The Landscape exporter constructs texture UVs from grid XY and writes those
values directly into the FBX UV array. Its weightmap UVs are likewise written
directly after multiplying by reciprocal grid-vertex dimensions. Sources:
[FbxMainExport.cpp:5960](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Editor/UnrealEd/Private/Fbx/FbxMainExport.cpp:5960>),
[weightmap scale:5887](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Editor/UnrealEd/Private/Fbx/FbxMainExport.cpp:5887>).

By comparison, ordinary StaticMesh export explicitly writes
`FbxVector2(VertexUV.X, -VertexUV.Y + 1.0)` for FBX, both in its mesh-description
and render-data paths. Sources:
[StaticMesh description export:4986](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Editor/UnrealEd/Private/Fbx/FbxMainExport.cpp:4986>),
[StaticMesh render-data export:5311](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Editor/UnrealEd/Private/Fbx/FbxMainExport.cpp:5311>).

The checked Interchange FBX mesh importer converts to
`FVector2f(UVVector[0], 1.0f-UVVector[1])`, with a source comment stating that
this flips UV Y for DirectX. It makes the same conversion for vertex-instance
UV assignment. Therefore ordinary StaticMesh round-trip applies the inverse
pair, while the Landscape export path leaves a V inversion plus an offset.
Source:
[FbxMesh.cpp:943](<C:/Program Files/Epic Games/UE_5.7/Engine/Plugins/Interchange/Runtime/Source/Parsers/Fbx/Private/FbxMesh.cpp:943>),
[vertex-instance assignment:1094](<C:/Program Files/Epic Games/UE_5.7/Engine/Plugins/Interchange/Runtime/Source/Parsers/Fbx/Private/FbxMesh.cpp:1094>).

For texture UV0, the `+1` offset is not harmless when a material later divides
coordinates by8: it becomes an additional one-eighth texture-period phase
shift. For the 0..1-like UV1 channel, the same operation also visibly flips
orientation. Native Landscape weight coordinates and exported WeightmapUVs
remain distinct mapping conventions; this diagnosis does not establish UV1 as
the current 512-square world-window bake's exact sampling contract.

## Correction to validate later, with permission

The intended authoring-pipeline correction has two independent parts:

- Remove the unintended import yaw for the Landscape-derived mesh, provided
  the intended geometry placement is the native Landscape's world orientation.
  Existing placements or hand adjustments that compensated for the old asset
  would need comparison.
- Make the Landscape FBX UV handoff use the same convention as ordinary
  StaticMesh FBX export. Conceptually, FBX should contain `(u,1-v)` before an
  importer that performs `(u,1-v)`, so the imported UVs recover `(u,v)`.
  This can be handled in the authorized mesh preparation/conversion step;
  no engine-source modification is implied or required by this diagnosis.

Changing yaw alone predicts geometry-axis correction while retaining
`UV0=(nativeU,1-nativeV)`. Flipping V alone leaves the quarter-turn geometry.
Both must be addressed to reproduce native UV orientation and phase.

A future temporary corrected copy should verify known asymmetric geometry
landmarks, UV0 at corresponding corners, an 8-metre textured span and normal-map
orientation. A final visual comparison should use the production material
settings chosen for the project, with the previously identified ORM color-space
decision explicit. No correction or experiment has been performed here.
