# Next authoring changes after material export and mask refresh

These are investigated implementation directions, not implemented behavior.
Keep the owner's fresh-Landscape acceptance test pending.

## First mask creation

Current refresh requires an imported mask with an existing PNG source. Add an
explicit creation action for a selected Landscape using an assigned material
instance, so no shared master is modified silently.

- Reuse capture/coverage/encoding from `TGATerrainMaskExport.cpp`; the create
  wrapper should take a new PNG filename, resolution and Unreal asset path.
  Derive initial bounds from the fully loaded Landscape extent. Keep the
  unrotated native Landscape restriction for the first version.
- Refuse existing PNGs and assets before capture/import; do not replace another
  Landscape's mask. Import using `UAssetImportTask` / `IAssetTools::ImportAssetTasks`
  with `bSave=false`, `bReplaceExisting=false`, `bAsync=false`.
- Make the imported texture linear (`SRGB=false`) and retain source filename
  provenance. Do not assign material metadata if import fails; report the
  generated PNG as a recoverable artifact.
- Copy inherited export metadata onto the assigned instance before setting its
  new mask/bounds. If a normal `SplatMask` parameter exists, set that instance
  override too; otherwise the ordinary parameter wins over the new metadata.
- Leave texture/material packages dirty for normal owner Save. Verify initial
  creation, effective mask export, refusal without mutation, then repaint via
  the existing refresh command. No StaticMesh prerequisite for mask creation.

Installed UE source references: `Editor/UnrealEd/Public/AssetImportTask.h`,
`Developer/AssetTools/Public/IAssetTools.h`, and existing
`Command_EditTerrainMaterial` for inherited metadata copying.

## Runtime DDS names

`PrepareTerrainTextures.ps1` still writes fixed `T_Splatmap_LevelWeights` and
`T_Splatmap_TransitionNoise` names. AGP resolves texture assets by source-filename
stem, so newly named mask sources require matching DDS names. Add optional
`-MaskName` / `-NoiseName` arguments while preserving current defaults; validate
safe unique output stems and keep exact pixel/mipmap verification. The creation
action should report the corresponding conversion command. Do not silently
change names for existing fixture users.

## Mesh reimport and UV correction

Prefer an explicit **Reimport Terrain Mesh and Correct UVs** wrapper over a
global import callback. Use successful reimport as the boundary, preserve import
yaw -90, and correct only the approved UV channels. Relevant installed APIs:

- `UStaticMesh::GetMeshDescription`, `ModifyMeshDescription`,
  `CommitMeshDescription`, `Build` in `Engine/Classes/Engine/StaticMesh.h`.
- `FStaticMeshAttributes::GetVertexInstanceUVs` in
  `StaticMeshDescription/Public/StaticMeshAttributes.h`.
- `FStaticMeshOperations` for any required tangent recomputation.
- `EditorReimportHandler.h` for successful reimport notification.
- `InterchangeGenericMeshPipeline.h` for later scoped import integration.

A persistent "corrected" boolean is insufficient: a fresh reimport needs the
correction again, while an already corrected mesh must not flip twice. Store
explicit source-convention opt-in plus raw/corrected UV fingerprints. Reject
ambiguous or already precompensated FBXs from `CorrectLandscapeFbx.py`; UV values
alone do not reveal the intended convention.

Verify positions, triangle connectivity, material slots and import transforms
unchanged; only approved UVs change. Check repeated action idempotency, same-file
and changed-topology reimport, corrected runtime FBX export, and normal/tangent
appearance. Do not migrate existing authored assets automatically.
