# Same-StaticMesh corrected-FBX reimport experiment

Observed 2026-10-23. **Passed** in an isolated Unreal 5.7.4 host on RTX 3070,
DirectX 11 SM5. This was an experiment, not a production menu implementation.

## Question and verdict

Can the strict FBX UV corrector feed a new source path into an existing
Interchange-imported terrain StaticMesh without replacing its UObject, changing
its geometry/material assignments or losing its intentional yaw -90?

**Yes for the audited mesh and unchanged raw geometry.** Two independent corrected
exports from the same raw file were reimported into the same temporary duplicate.
Both retained all vertex positions, triangle connections, material slots and
assignments, and pipeline yaw. Repeating the workflow did not flip UVs twice.

## Inputs and isolation

- Source asset copied into the automation host:
  `/Game/Meshes/Landscape/Test/SM_Landscape_Test.SM_Landscape_Test`.
  Its `MI_Landscape` dependency was copied too; the host already held the master,
  function and nine snow textures.
- Raw FBX: AGP-Content `Content/Meshes/Landscapes/Test/SM_Landscape_Test.fbx`,
  MD5 `18ba5d2bc52b9808e6c2f7f522e18417` before and after the experiment.
- Corrector: `Tools/Terrain/CorrectLandscapeFbx.py`, run with Blender 5.1's
  bundled Python using `-B` and its `io_scene_fbx` module folder. The strict
  verifier accepted both outputs. The raw filename was preserved inside two
  distinct output folders, `Intermediate/TerrainMeshReimportExperiment/First`
  and `Second`.
- The test duplicated the copied asset into a unique `/Temp/TGATerrainMeshReimport_*`
  package with the same mesh name. No source asset, source level or package was
  saved. The real editor process remained open and untouched.

## Reimport seam

The test used the exact call below; no replacement default pipeline was supplied:

```cpp
FReimportManager::Instance()->Reimport(
    Mesh, false, false, CorrectedPath, nullptr, 0, false, true, false);
```

`PreferredReimportFile` switches the existing source path even when a source is
already recorded. `SourceFileIndex=0`, automated import and disabled force-new-file
dialogs kept the operation unattended. Stored Interchange pipelines were reused.
Yaw was read from their `ImportOffsetRotation` struct through reflection.

## Results

| Check | First corrected source | Fresh second corrected source |
| --- | --- | --- |
| Same UObject and package | Pass | Pass |
| 4096 vertex positions | Exactly unchanged | Exactly unchanged |
| 6272 triangle vertex-instance connections | Exactly unchanged | Exactly unchanged |
| 18816 corners in UV0 and UV1 | Retained | Retained |
| U unchanged, V = 1 - original V | Max error 5.960464477539063e-08 | Same |
| Slots, imported slot names, material pointers | Retained | Retained |
| Stored pipeline yaw | -90 degrees | -90 degrees |
| UV arrays compared with first corrected import | N/A | Exactly equal |

The copied source asset's positions, UVs and dirty flag remained unchanged.
The new provenance guard also rejected using the first corrected output as a
raw source and wrote no rejected output.

## Evidence and rerun

- Automation report: `Intermediate/TerrainMeshReimportExperiment/AutomationReport/index.json`
  records one successful check, zero test warnings/failures.
- Numeric output: `Intermediate/TerrainMeshReimportExperiment/reimport-result.json`.
- Engine log: `Intermediate/TerrainMeshReimportExperiment/Automation.log`.
- Host-only throwaway test source:
  `Intermediate/TerrainExporterInstanceCheck/HostProject/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/Tests/TGATerrainMeshReimportExperiment.cpp`.
  Its host-only Build.cs adds `MeshDescription`, `StaticMeshDescription` and
  `InterchangeEngine`; do not copy those changes into unrelated production code.
- Build that host using the documented `Build.bat UnrealEditor Win64 Development`
  command, then run its `UnrealEditor-Cmd.exe` with `-unattended -nop4 -nosplash
  -RenderOffscreen -d3d11 -nosound`, automation filter
  `TGA.TerrainMeshExperiment.SameMeshReimport` and `-TestExit="Automation Test Queue Empty"`.

The host files and machine-local logs are disposable and ignored. This report
captures their verdict for continuation. Changed sculpt/topology, failed reimport
recovery, runtime Front-X FBX export, and final tangent/normal appearance were not
tested here; the owner's complete from-zero authoring acceptance remains pending.

## Changed-geometry fixture prepared for production tests

After the initial experiment, a disposable generator was added at
`Intermediate/TerrainMeshReimportExperiment/MakeChangedRawFixture.py`. It uses
Blender's `fbx2json.py` / `json2fbx.py` round trip to change exactly one coordinate
in the raw `Vertices` array by +25. A parsed comparison permits only that change
plus writer FileId/CreationTime, and confirms all raw UV arrays remain unchanged.
The original raw file's SHA256 remains
`0ea2e05ef3143bf1510dd477ad2f0c8f50f0af158db40bdded265070e1a75eed`.

Generated raw fixture:
`Intermediate/TerrainMeshReimportExperiment/ChangedRaw/SM_Landscape_Test.fbx`.
Its `.fixture.json` records the verification; it deliberately has no correction
provenance. Generate a fresh copy by running the fixture generator with Blender's
bundled Python `-B`, then three arguments: original raw FBX, new fixture filename,
and the `io_scene_fbx` module directory. Existing fixture output is refused.

Production test `TGA.MeshImport.TerrainReimport.ChangedSculpt` takes
`-TerrainChangedRawFbx=<fixture>` in addition to `-TerrainRawFbx` and
`-TerrainCorrector` when running outside the content project. It is intended to
prove one updated imported position, stable identity/connectivity/slots/yaw,
and no extra UV reflection. Its execution result belongs in ticket 07; it was
not part of the initial passing experiment above.

## Fixture units and renamed-node verification

The first production sculpt check exposed an incorrect test expectation, not a
failed geometry update: exactly one vertex changed, but the raw coordinate delta
is not measured directly in imported centimetres. Independent inspection of the
raw FBX shows `GlobalSettings.UnitScaleFactor=1.0` centimetre and the sole mesh
Model's `Lcl Scaling=(100,100,100)`; geometric scale is the FBX default 1. The
stored Interchange `ImportOffsetUniformScale` is 1. Therefore the fixture's +25
raw-coordinate change must produce **2500 cm**, derived as `25 * 1 * 100 * 1`.
The generator now records those inputs and expected centimetre displacement in
`.fixture.json`. The test checks the import scale assumption and prints actual
and expected displacement in its assertion. The existing `ChangedRaw` fixture's
metadata was updated from the independently verified regenerated fixture at
`ChangedRawUnitsVerified`; its FBX bytes were not overwritten.

A second mode, `wrong-node`, creates
`Intermediate/TerrainMeshReimportExperiment/WrongNodeRaw/SM_Landscape_Test.fbx`.
It changes only the Model name from `Landscape` to `UnmatchedLandscape`, plus
writer FileId/CreationTime; geometry and raw UV arrays remain identical. It has
no UV-correction provenance. Pass it through `-TerrainWrongNodeFbx=` to
`TGA.MeshImport.TerrainReimport.RenamedNode`. Execution showed that Interchange
legitimately matches this single renamed mesh and updates its UVs; rejecting it
unconditionally was an incorrect test assumption. The production test now checks
that success means a real UV update with retained geometry/slots/yaw and a new
prepared source path. The helper separately requires the selected mesh in the
completed async import result, rather than trusting the synchronous bool.

Final production checks, including changed sculpt and renamed node, passed in
`Intermediate/TerrainSchoolVerified/index.json` with zero warnings/failures.
