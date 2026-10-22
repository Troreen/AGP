# Temporary FBX UV correction procedure

## Completed test: preserve intentional yaw -90

Latest owner clarification on 2026-10-22 supersedes the zero-rotation proposal
below: yaw -90 is required. The owner approved creating a scratch UV-corrected
FBX, importing it separately with that yaw, and writing the investigation results.
Production assets and the DDS experiment are outside that approval.

Executed [scratch writer/verifier](correct-scratch-fbx.py). Its first attempt
stopped before writing because Windows uses a different int32 array typecode;
the script now explicitly converts it. The next attempt wrote the copy, then
the bundled parser rejected reuse of its initialized namespace. A fresh parser
namespace verified that existing output without rewriting it. Parsed differences
are exactly FileId, CreationTime and the two intended UV arrays; every other
property is identical. The source MD5 remained
18ba5d2bc52b9808e6c2f7f522e18417, including after the Unreal import.
See [semantic verification](audit-corrected-fbx-semantic.json).

Imported into `/Temp/SplatmapInvestigation/UVCorrectedYawMinus90_20261022/`
using a new transient Interchange pipeline. Copied the original pipeline's
documented writable options, including yaw -90, translation, scale, mesh and
normal/tangent settings; disabled material/texture import and changed only the
test name/folder options. Deprecated `import_collision` was skipped in favor of
its copied replacement `collision`. Used automated async import with
replace_existing=False; no save call and no level placement.

All 4096 vertex positions match the original exactly; all 6272 triangles retain
the same vertex-instance connections. All 18816 corners satisfy:

```text
corrected UV0 = ((2800-localY)/100, (localX+2800)/100)
corrected UV1 = ((2800-localY)/5700, (localX+2800)/5700)
```

UV0 U is unchanged and V equals 1 minus the original imported V exactly.
UV1 differs from the same relation by at most 5.97e-8 due to float precision.
This removes the export/import V reflection while retaining the mesh rotation.
Recomputed normals and Mikk tangents remain enabled. This is numeric mesh-data
verification, not final surface/normal-map appearance or AGP importer validation.
See [complete import measurements](audit-uv-corrected-yaw-minus90.json).

The original import yaw remains -90. The level was already dirty before the
test; its dirty state was not cleared or saved. Dirty content queries were empty
before and after. The expected C:/Temp/... temporary uasset path was absent.
The scratch FBX and reports remain in this feature directory; no staging,
commits, deployment or Perforce actions occurred.

## Historical proposal (rotation choice superseded)

Prepared 2026-10-22. **Proposal only at this stage.** No corrected FBX has been
written by this investigator. Root is awaiting permission for the scratch-copy
experiment; source FBX, Unreal assets and saved import settings stay untouched.

The approved zero-rotation import already shows the geometry issue is corrected
by clearing the saved yaw offset. Its remaining UV0 is
`((X+2800)/100, 1-(Y+2800)/100)`. A conventional FBX UV precompensation should
make the importer produce the native grid coordinates without a shader change.

## Existing tools, no DCC re-export

- Interpreter:
  `C:/Program Files/Blender Foundation/Blender 5.0/5.0/python/bin/python.exe`.
- Python module folder:
  `C:/Program Files/Blender Foundation/Blender 5.0/5.0/scripts/addons_core/io_scene_fbx`.
- Read parser: `runpy.run_path(folder + '/fbx2json.py', run_name='fbx_reader')`,
  then returned namespace `parse(str(source))` gives `(root, version)`.
- Writer: put the module folder in `sys.path`, `import encode_bin`, recursively
  clone parsed elements into `encode_bin.FBXElem`, then
  `encode_bin.write(str(scratch_destination), encoded_root, version)`.
- Run Python with `-B` to avoid bytecode-cache writes. Importing encode_bin was
  checked read-only; its NumPy dependency is already present.

Avoid a text JSON round trip: json2fbx.py rewrites `::` in string properties to
FBX separators. Direct typed cloning preserves the existing raw byte strings.
The current source's property types are exactly I,S,R,D,L,d,i,C; supported writer
methods are `add_int32`, `add_string`, `add_bytes`, `add_float64`, `add_int64`,
`add_float64_array`, `add_int32_array`, `add_char` respectively.

## Smallest controlled transformation

Read the original 85,632-byte source, MD5
`18ba5d2bc52b9808e6c2f7f522e18417`, from content-owned
`Content/Meshes/Landscapes/Test/SM_Landscape_Test.fbx`.
Refuse to write to that source path or an existing destination. Resolve output
under AGP `.scratch/terrain-splatmaps/` before writing.

Recursively clone all elements, names, types and property values. Only at these
two array nodes replace each odd-index value V with `1-V`, leaving U untouched:

```text
/Objects/Geometry/LayerElementUV[0]/UV         # TextureUVs / surface UV0
/Objects/Geometry/LayerElementBumpUV[0]/UV     # WeightmapUVs / imported UV1
```

Assert exactly two arrays were selected, both float64 with 8,192 scalar values,
and retain FBX version 7300. Keep geometry positions, indices, material records,
model translation/scaling, axes, normal/tangent/binormal arrays and connections
unchanged. This isolates UV convention from geometry conversion or DCC defaults.

**Writer metadata exception:** `encode_bin.write` calls
`_write_timedate_hack` (`encode_bin.py:347-376`), replacing top-level FileId and
CreationTime with fixed values, and rewrites the binary footer/alignment. Array
compression and offsets are rewritten as well. Therefore this is a semantic
UV-only geometry edit, not a claim that only UV bytes differ. Record these known
metadata changes and compare parsed trees rather than full-file byte equality.
There is no reason to patch the installed writer or suppress its metadata logic
for this disposable experiment.

## Required evidence after permission

1. Parse scratch output anew. Compare every element path/type/property to source,
   allowing only the two transformed UV arrays plus FileId/CreationTime. Verify
   original source checksum is unchanged. Refuse further import if other data
   differs.
2. Root imports the scratch file into a new temporary package with zero rotation,
   no material/texture import, and no save. Keep the old source assets intact.
3. Measure all vertex/UV values at the same existing seam. Expected imported UV0
   is `((localX+2800)/100, (localY+2800)/100)`; UV1 is the same numerator divided
   by 5700, ranges0..56/57. Geometry must match the zero-rotation control.
4. Do not call this complete normal-map parity. See tangent qualification below.
   This experiment establishes a viable export/import UV convention first.

## Tangent and UV1 qualifications

V reflection reverses the V derivative and bitangent handedness. Positions and
geometric normals need not change; an imported tangent frame can nevertheless
become inconsistent if inherited unchanged from old UVs. Preserve tangent data
in the scratch copy to keep the coordinate experiment controlled, then inspect
actual import/build tangent settings and output. A final corrected asset should
derive a consistent frame from corrected UVs, such as an explicitly recomputed
MikkTSpace basis, or update bitangent orientation consistently. Do not flip normal
texture green merely to mask an unmeasured frame discrepancy.

Root subsequently queried both original and zero-rotation control common-mesh
settings live: bake_meshes=True, bake_pivot=False, recompute_normals=True,
recompute_tangents=True, use_mikk_t_space=True, compute_weighted_normals=True,
use_full_precision_uvs=False. Thus preserving that configuration for the
UV-corrected temporary import will recompute Mikk tangents from corrected UVs;
no blind green-channel flip is warranted. Imported frame inspection and a normal
appearance comparison remain distinct from proving numeric UV correction.

Precompensating BumpUV too follows the same conventional FBX round-trip rule and
preserves source UV1 after the importer's own flip. Its denominator57 is not
the desired512x512 baked world-mask transform. Passing the UV1 round trip does
not authorize sampling the new weight bake with UV1 directly.
