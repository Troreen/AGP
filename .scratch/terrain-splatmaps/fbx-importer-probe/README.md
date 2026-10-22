# Throwaway TGAFBX importer probe

Uses the actual bundled `TGA::FBX::Importer::LoadMesh` API, default options:
keep imported normals, merge duplicate vertices. No engine references.

Build from the AGP root:

```powershell
& 'C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe' .scratch/terrain-splatmaps/fbx-importer-probe/FbxImporterProbe.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo /nr:false
```

The project links `Dependencies/TGAFBXImporter/lib/Debug/TGAFbx.lib` and copies
the matching `libfbxsdk.dll` beside the executable. It uses the Debug DLL CRT,
C++20, v145, and Windows SDK 10.0. Object-embedded debug information and no
linker debug output avoid the PDB-service wait observed in this environment.
All outputs stay in this probe folder's `bin/` and `obj/` directories.

Run:

```powershell
& .scratch/terrain-splatmaps/fbx-importer-probe/bin/FbxImporterProbe.exe path/to/mesh.fbx
& .scratch/terrain-splatmaps/fbx-importer-probe/bin/FbxImporterProbe.exe path/to/mesh.fbx --all
```

Default JSON includes bounds, counts, original axis/unit enum values, element
transforms and selected samples. Samples include position extrema and two
asymmetric UV targets. `--all` includes every vertex. Each sample includes UV0,
UV1, position, normal, tangent and binormal. Failure messages go to stderr and
return nonzero. Probe is intended for static non-LOD exports.

## Verified controls and runtime export

- `original-all.json`: original runtime `Content/Meshes/Landscapes/Test/SM_Landscape_Test.fbx`.
- `corrected-raw-all.json`: UV-corrected raw FBX before Unreal re-export.
- `runtime-all.json`: REJECTED ordinary Unreal StaticMesh export with
  `force_front_x_axis=False`. Its world geometry orientation differs from the
  native scene convention despite identical symmetric bounds.
- `frontx-all.json`: ACCEPTED ordinary Unreal StaticMesh export of the corrected
  temporary asset with `force_front_x_axis=True`, retaining the intentional
  import yaw -90 in the mesh geometry.
- `plane-all.json`: `Content/Meshes/Debug/SM_Plane.fbx`.

All imports succeeded. The runtime terrain has 3249 merged vertices and 18816
indices. Actual importer local bounds are `[-2800,0,-2800]` to
`[2800,24.21875,2800]`, with identity local/global/geometric transforms. The
plane is `[-100,0,-100]` to `[100,0,100]`. `bounds.json` contains both in the
requested scene-verification schema.

Across all 3249 accepted FrontX runtime vertices:

```text
UV0 = ((2800 - AGP.X) / 100,  (AGP.Z + 2800) / 100)
UV1 = ((2800 - AGP.X) / 5700, (AGP.Z + 2800) / 5700)
```

Maximum errors: UV0 `1.066e-14`, UV1 `4.229e-8` (float precision).
`frontx-registration.json` records the numeric check. Every corresponding
position also matches the original raw terrain FBX through the actual importer,
maximum XYZ error `9.095e-13`. This includes asymmetric elevations; for example,
AGP position `[-100,10.15625,300]` has UV0 `[29,31]`. Comparing corresponding
Unreal samples gives AGP `(X,Y,Z) = (Unreal.Y, Unreal.Z, Unreal.X)`, matching the
scene convention. `force_front_x_axis=True` is therefore required for the
runtime StaticMesh export. Preserve the import yaw -90; do not compensate for
the rejected export in the shader. World mask registration must still follow
the native Landscape world mapping, separately from these mesh UV formulas.

The rejected False export's formulas and checks remain in
`runtime-registration.json` for diagnosis only. Symmetric terrain bounds alone
were insufficient to detect its wrong orientation.

This probe verifies actual importer coordinates, units and attributes. It does
not establish final material appearance or authoring correctness.
