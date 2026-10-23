# Create and import the first painted Landscape mask

Status: resolved
Blocked by: 05

Add an explicit TGA action for a selected native Landscape with an assigned
MM_Landscape material instance. Reuse existing capture and coverage validation,
derive initial full loaded Landscape bounds, and create a new named PNG at
512 square pixels by default. Import it as a linear lossless texture without
saving assets; configure the instance's own mask metadata and any normal
SplatMask parameter override. Preserve shared parent metadata.

Refuse existing output files/assets before capture. Failed import must leave
material metadata unchanged and report the recoverable PNG. No StaticMesh,
JSON patch, pre-existing mask, automatic DDS deployment or package save required.
Expose capture resolution as an API input for tests and later UI extension.

Verify through fresh-Landscape GPU tests: first mask creation/import, effective
instance assignment, correct bounds/source resolution, no shared-parent changes,
refused file/asset collisions, and subsequent refresh using the new mask.
Then support matching custom DDS output stems in the preparation script.

Current fixed bases: AGP 0857005; AGP-Content 4a3fb55. Both clean at start.
No new commits/pushes until a verified checkpoint or user instruction.

## Verification

Unreal 5.7.4 isolated plugin build passed. All fifteen `TGA.Terrain` GPU checks
passed with zero test warnings/failures on RTX 3070 / D3D11 SM5; final report
`Intermediate/TerrainBootstrapFinal/index.json`. An executable red test first
failed against the unimplemented creation entry point. Creation tests cover
no prepared metadata, imported numeric RGB, normal parameter precedence,
inherited control preservation, independent PNG/package collisions and refresh.

Default, custom and digit-leading/hyphenated DDS output names passed exact RGBA
and full-mipmap checks. Unsafe/duplicate names and unintended overwrite were
rejected before publication. Reports under `Intermediate/Terrain*NamedTextures`.

Independent standards review found no confirmed violation. Spec review found
creation accepted names DDS preparation rejected; aligned both contracts and
verified `7_New-LandscapeWeights`. Reviewer confirmed the fix. No authored source
asset or package was saved. No initial-mask UI interaction is claimed: the
actual editor's plugin must be rebuilt/restarted before owner walkthrough.
