# Prepare raw Landscape FBX and reimport the same mesh with corrected UVs

Status: resolved
Blocked by: 06

## Question before implementation

Can the strict existing FBX corrector feed a normal Interchange reimport into
the same StaticMesh without changing positions, triangle connectivity, material
slots or the intentional -90 yaw? Run the isolated real-mesh experiment first;
do not save/migrate any authored asset or install into the open editor.

## Intended behavior

One explicit TGA command on the selected existing terrain mesh selects the raw
FBX, creates a new strict UV-precompensated file, validates its provenance, and
reimports that file into the same mesh through its existing pipeline. No global
UV flip, default replacement pipeline, package save or shader compensation.
Raw input and previous generated outputs remain unchanged.

Known corrected source provenance must be rejected before preparation; malformed
or stale correction provenance must produce an actionable error. Repeating the
operation from the original raw source must not double-flip. Retain local tool
paths outside tracked assets/settings. Prefer the existing Blender FBX parser
and writer rather than another serializer with weaker preservation guarantees.

## Verification

The isolated real-mesh experiment passed, then the production helper/UI compiled
and passed four `TGA.MeshImport.TerrainReimport` checks. Final integrated report
`Intermediate/TerrainSchoolVerified/index.json` records 20 successes and zero
warnings/failures: 19 production checks plus the earlier isolated experiment.

The same mesh retains 4096 positions, 6272 triangles, both UV channels, material
assignments and yaw -90 for unchanged raw geometry. Repeated raw reimport does
not double-flip. A one-vertex sculpt update reaches the same mesh; the independently
derived displacement is 2500 cm (25 raw units * model scale 100 * 1 cm units).
A renamed single source node legitimately reimports and must update UVs; it is
not assumed to be a failed match. Preflight covers missing source, wrong yaw,
known/stale/malformed correction provenance and external preparation failure.

Spec review found Unreal's convenience bool can report success without the
selected mesh being imported. The helper now waits for ReimportAsync, rejects
error results/incomplete status and requires the exact mesh in imported objects.
Reviewer confirmed the source fix. Standards review found no confirmed defect.
The two Python guard tests pass with Blender's bundled Python using `-B`.

No source package was saved by the agent. Owner separately approved including
their current MM_Landscape and Lvl_Splatmap_Testing edits in the school checkpoint.
Final owner appearance, runtime Front-X export and complete walkthrough remain
pending. The user explicitly declined normal scene-export coverage validation;
that proposal and its test were removed. Bases: AGP0091d15, content60f00b5.
