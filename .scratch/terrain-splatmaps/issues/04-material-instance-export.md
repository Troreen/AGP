# Export effective terrain material-instance parameters

Status: resolved
Blocked by: none

## Scope

Resume authorized 2026-10-23. AGP base `09f3e1771941a3151324ce222c36aec2efd71f52`;
AGP-Content base `8cd98950fa1805c68e7be88a854dcb560435c771`. Both trees started clean.
Use the existing `TGA.TerrainExport` serializer and normal scene-export seams.

Normal Unreal scalar/vector/texture parameters, resolved through the material
instance's parents, must take precedence over legacy terrain export settings
for the supported eleven texture roles, `SurfaceMapping`, `NoiseControls` and
`DEBUG`. Mask world bounds remain export metadata and determine `MaskU/MaskV`.
Missing graph parameters fall back to existing prepared settings for compatibility.
An explicitly null or invalid graph value must fail validation rather than
silently reverting to the legacy value. Validate the effective values.

Keep ordinary materials and the runtime parameter schema unchanged. Preserve
the existing mesh import rotation and all authored assets. Investigate exposing
the source master's hardwired textures as normal parameters without replacing
team materials in place.

## Acceptance

- Parent values and child overrides reach exported JSON once per runtime role.
- Clearing a child override restores inherited values.
- Invalid effective texture/numeric values abort export with an actionable error.
- Old prepared materials without these graph parameters still export.
- Mask bounds remain authoritative even if graph parameters share those names.
- Existing and new Unreal automation checks pass; document limitations and how
  an artist sets the supported values. Final from-zero owner test remains pending.

## Verification

Unreal 5.7.4 isolated plugin build passed. All ten `TGA.TerrainExport` checks and
`TGA.TerrainAuthoring.CopiedMaster` passed; local report:
`Intermediate/TerrainFinalInstanceChecks/index.json`. The original precedence
bug and the review-discovered explicit-null child override both had executable
failing regressions before their fixes. Existing AGP MaterialImportTests passed
on the RTX 3070 after a Debug x64 build.

Independent spec review found the null-override defect, then confirmed its fix.
Independent standards review found no confirmed violation; its optional
connection-record suggestion was deferred because the local arrays are appended
together and covered by connection-preservation tests. No authored asset saved.
The actual editor's loaded plugin remains old and must be rebuilt after closing.
