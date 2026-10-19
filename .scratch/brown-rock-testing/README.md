# BrownRock08 snow material

The shader test scene now has a separate `BrownRock08_TestSphere` using
`MM_BrownRock08`. The original sphere, snow material, ground, and lights are
unchanged. The new sphere is offset 260 units along exported X from the old one.
The user visually accepted the plain-rock baseline. The new material now adds
top-facing snow using the original snow material's world-normal slope mask.
Rock normal-map detail shapes the coverage edge; snow blends the base color,
AO, roughness, metallic, and normal together. This is a surface layer, with no
geometry displacement or time-based snowfall simulation.

## Assets and conversion

Source: the user's downloaded `BrownRock08_2K` PNG package. Original dimensions
are 2048 x 2040 and are preserved. Runtime assets live in
`Content/Textures/BrownRock08/`:

- BaseColor: BC1 sRGB, with mipmaps filtered in linear color space.
- Normal: BC5 linear RG, with normalized normal mipmaps. Source green-channel
  orientation is preserved; it has not been visually confirmed against the height map.
- OcclusionRoughnessMetallic: BC1 linear, R=AO, G=roughness, B=metallic.
- The converter can also produce a height map. It is not included in the
  runtime assets because this shader does not use it.

Regenerate from the repository root with Python, Pillow, and numpy:

```powershell
python .scratch/brown-rock-testing/convert_material.py 'C:/Users/tarik/Downloads/BrownRock08_2K'
```

The converter supports `--flip-normal-green` if visual comparison establishes
that the downloaded normal uses the opposite Y convention.

## Material controls

Edit `Source/Engine/GraphicsEngine/Shaders/MM_BrownRock08.hlsli` for shader logic.
Its scene record supplies white tint, tiling 1, and roughness/metallic multipliers
of 1, preserving the downloaded maps. Metallic remains zero because the map is
zero. The scene's base Roughness and Metallic values act as multipliers for this
particular shader. `Tiling` is an exported scalar parameter.

Snow controls are exported parameters in the same scene material record:

- `SnowAmount`: 0 restores plain rock; 1 enables the full slope-based layer.
- `SnowRoughness`: roughness of covered areas, default 0.85.
- `SnowFlatness`: how much covered areas hide the rock normal map, default 1.
- `SnowColor`: linear RGBA tint, default (0.95, 0.97, 1, 1); RGB is used.

The slope mask is `saturate(normal.y * 4.5 - 2.5)`, multiplied by SnowAmount.
AGP uses Y-up. Covered areas blend toward AO 1 and metallic 0.

Build after shader edits so the new include is copied to `Bin/Debug/EngineShaders`.
Restart the game after material or scene changes to ensure assets are recreated.
An Unreal re-export can overwrite these manually authored scene additions.

## Verification

- Game.sln Debug x64 build passed with VS v145; existing unrelated warnings remain.
  The initial sandbox build failed in MSBuild FileTracker with access denied;
  the approved build outside the sandbox succeeded.
- Custom material compiled through Lit_PS, GBuffer_PS, Unlit_PS, and Surface_VS.
- DDS data checked against source PNGs: channel packing matches, metallic remains
  zero, and full-resolution height values match exactly. All maps have 12 mip levels.
- `git diff --check` passed.
- Plain-rock runtime appearance was accepted by the user.
- Snow material compiled through all four shader paths; snow runtime appearance
  is pending the user's visual check.

## Visual check

Run `Bin/Debug/Game.exe`. The configured initial scene is the shader test scene.
Find the added rock sphere alongside the existing snow sphere. Confirm snow on
upward-facing areas, brown rock on steeper sides, and a detailed coverage edge.
Use F5/F6 debug views to inspect albedo, roughness, metallic, AO, and world normals.
Exposed rock should have varied roughness/AO; snow should be rough with less
rock normal detail. Both should have black metallic. Check the existing
snow sphere and gray ground still look as before. Normal-map direction and texture
scale can be adjusted after this first look.
