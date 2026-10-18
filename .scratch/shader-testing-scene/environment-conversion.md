# Fixed hillside cubemap

Source: `Content/Textures/snowy_hillside_02_4k.hdr`, preserved unchanged. Its file signature and decoder header identify **OpenEXR**, despite the `.hdr` suffix: 4096 × 2048, linear float32 RGBA, RGB range `0.0302073..2.02448`.

## Rebuild

From the repository root in PowerShell:

```powershell
python -m pip install numpy OpenEXR
python .scratch/shader-testing-scene/convert_environment.py
```

Verified with Python 3.11, NumPy 2.4.2 and OpenEXR 3.5.2. These are offline conversion dependencies only; the game needs neither package.

Output: `Content/Textures/snowy_hillside_02_cube.dds`, 512 × 512 per face, DX10 `R16G16B16A16_FLOAT`, six faces and ten ordinary box-filtered mip levels. File size: 16,777,348 bytes. No exposure adjustment, tonemapping, sRGB conversion or prepared diffuse/specular IBL is applied. Half-float storage retains source HDR values; sampled output RGB range is `0.0339050..1.8388672` after quantization.

## Orientation

The converter writes standard D3D face order `+X, -X, +Y, -Y, +Z, -Z`. For face coordinates `u,v` in `[-1,1]` with `v` increasing downward, directions are `(1,-v,-u)`, `(-1,-v,u)`, `(u,1,v)`, `(u,-1,-v)`, `(u,-v,1)`, and `(-u,-v,-1)`. World `+Y` maps to the panorama north pole; world `+Z` maps to its horizontal center. Longitude wraps and latitude clamps during bilinear sampling.

The generated `environment-faces.png` is a left-to-right contact sheet in that face order. Reinhard mapping and display gamma are applied **only to this preview**. Visual inspection confirmed upright side faces, sky on the top face and snowy ground on the bottom face. The runtime still needs to confirm skybox/reflection alignment with the same direction convention.

## Verification

- Parsed DDS and DX10 headers: dimensions 512 × 512, mip count 10, texture2D resource dimension, cubemap misc flag, array size one, all six cube faces present.
- Exact byte count matches six complete face/mip chains of RGBA half-floats plus the 148-byte header.
- Every stored value is finite; values above one survive conversion.
- Original source is opened read-only and never rewritten.

This is an ordinary cubemap conversion. Per-face box mips intentionally preserve the engine's current approximation; seam-aware filtering and physically prepared IBL remain future work.
