# Splatmap integration checkpoint — 2026-11-06

The integration retains the Perforce engine/exporter behavior and restores the
reviewed Git splatmap pipeline. Git commits and pushes were authorized in the
implementation session. Perforce submission was not authorized or performed.

## Baselines and implementation

Engine baseline: `f619090a9e9719d890556ad39aa6943b7dc5770b`, parent `67ae452`.
Content baseline: `93b334e93e70ac18f018ec62363fed2a214342fa`, parent `3070f92`.
The existing SplatmapV2 and ExporterUpdateBranch branches retain feature history.
Baselines adopt the captured working Perforce revisions, not incoming updates.
Personal guidance, existing opened content edits and generated caches were
excluded. Full revision inventories and scoped diffs remain in local scratch.

Engine changes restore explicit texture-role binding with legacy fallbacks,
compiled constant-buffer offsets, storage bounds checks, affine mask helpers
and slope-aware projected normals. Team matrix transforms, instancing, particles,
skylights and Internal shader placement remain. Generated projects needed the
relocated tonemapping references and the existing DodgeAbility source entry.
MaterialUtilities now includes `../Internal/Common.hlsli`; its former include
could pick up a stale deployed Common and prevent the landscape material from
compiling. The material integration fixture covers this regression.

The exporter writes linear source-pixel RGBA DDS masks, preserves both landscape
switches, serializes landscape ID12 after camera ID11, omits disabled mask
references, checks writes and preserves valid empty custom objects. It retains
the team's omission of default Parent/Transform fields. Preparation accepts DDS
only, preserves ordinary scene data and accepts omitted DDS pitch when the
header does not set DDSD_PITCH. Ordinary texture conversion is unchanged.

## Observed verification

- Debug x64 solution build with v145: passed. Existing compiler/linker warnings
  remain, including the team Transform temporary-reference warning.
- Material import integration test: passed. Including MaterialUtilities first
  failed before its include-path fix, then passed after the fix.
- PaintedLandscapeRuntime regression script: passed, including exact RGBA/alpha
  placement, seams, scene/material preservation, missing masks, PNG rejection,
  DDS without pitch, invalid layouts and rectangular-grid hole rejection.
- Exporter package build from merged source: passed with Unreal 5.7.
- Unreal `TGA.WeightmapExport.DirectDDS`: passed, including unique proxy/slot
  names, source orientation, disabled references and empty custom payloads.
- Opt-in `TGA.SplatmapIntegration.FreshTesting2`: passed in an isolated editor,
  after native transient repaint. The initialized plugin exported four 64x64
  `_Linear.dds` masks and an 87,989-byte scene. The source map hash was unchanged;
  this acceptance paint was not saved to authoring assets.
- Fresh preparation produced a 127x127 runtime atlas and runtime-compatible JSON.
  AGP imported the prepared scene as 15 actors and two master materials.
- Fresh Unreal static-mesh import/re-export with full precision UVs and high
  precision tangent basis: AGP face/vertex normal alignment 0.999512 versus
  0.217846 for raw Landscape FBX. Matching 95,256 corners by position tolerance
  found maximum position difference 2.73e-12 cm and zero UV0/UV1 difference.
- Scoped deployment backed up differing files and copied nine runtime files.
  Game loaded the terrain mesh and compiled MM_Landscape. A computer-use capture
  visibly showed the terrain, surface detail, lighting and landmark cubes after
  correcting the shader include failure reported during runtime inspection.

## Remaining acceptance

Full visual parity, painted-boundary comparison with native Unreal, component
seam closeups, floater material acceptance and the final normal debug view remain
open. Keyboard-driven diagnostic checks alternated between the visible surface
and blank frames, so they do not establish normal-debug correctness. No claim of
full rendered acceptance is made. The user's alternative normal solution remains
deferred.

An existing untracked NativeMaterialImportTests suite failed with NO_SUCH_FIELD
in its fixtures; it was left unchanged. Actual prepared-scene import passed via
the rebuilt geometry verifier. The user's open Unreal editor was left untouched;
rebuilt binaries were tested in an isolated project rather than replacing its
loaded plugin. Restart/rebuild that editor before using its export menu.

The original application startup settings were restored on disk after testing.
Local evidence, binaries, absolute-path inventories and generated caches are not
part of the integration commits. No Perforce reconciliation was applied.
