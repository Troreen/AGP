# Terrain texture preparation evidence

Prepared in the engine checkout only. No external content files were changed.

Tool: `content-staging/Tools/Terrain/PrepareTerrainTextures.ps1` (candidate for
AGP-Content `Tools/Terrain/PrepareTerrainTextures.ps1`). Requires Windows,
System.Drawing and DirectXTex texconv with `--ignore-srgb` support.

Run from AGP root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .scratch/terrain-splatmaps/content-staging/Tools/Terrain/PrepareTerrainTextures.ps1 -MaskPng .scratch/terrain-splatmaps/diagnostic-weightmap-dunes-rough-cliff.png -NoisePng C:/Users/tarik.bergstrom/Perforce/AGP-Content/Content/Textures/T_Noise01.png -TexconvPath C:/Users/tarik.bergstrom/Perforce/AGP-Content/texconv.exe -OutputDirectory .scratch/terrain-splatmaps/texture-preparation
```

The process execution-policy option is needed on this machine; machine policy
was not changed. Successful test used DirectXTex version 2026.5.8.1.

| Output | Size | DXGI format | Mips | Exact total bytes |
| --- | --- | --- | --- | --- |
| `texture-preparation/T_Splatmap_LevelWeights.dds` | 512 x 512 | 28 (R8G8B8A8_UNORM) | 10 | 1,398,248 |
| `texture-preparation/T_Splatmap_TransitionNoise.dds` | 256 x 256 | 28 (R8G8B8A8_UNORM) | 9 | 349,672 |

Both textures have complete mip chains through 1 x 1. Numeric comparison of
every top-level RGBA channel byte against each source PNG found zero mismatches
in the raw DDS data and in the DDS-to-PNG round trip. This includes RGB below
the mask's zero alpha. Full results and source SHA256 hashes are in
`texture-preparation/verification.json`. Generated lower mips use linear BOX
filtering with alpha separate from color; noise wraps at its edges.

The source PNG hashes were unchanged. A second run without `-Force` failed
before conversion as intended; hashes of both original PNGs and both prepared
DDS files remained unchanged. Existing prepared files need explicit `-Force`.
Temporary working copies remain under `texture-preparation/.prepare-*` for
inspection. The tool never deletes original PNGs or invokes ExportAssets.bat.

This verifies conversion precision and storage, not runtime visual alignment,
noise transition appearance, or the final mask authored for the level. The mask
used here is the previously approved diagnostic weightmap.
