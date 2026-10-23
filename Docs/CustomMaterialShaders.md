# Custom material shaders

Each exported Unreal master material uses an individual `<MasterMaterialName>.hlsli`
in the configured shader folder (`Bin/Debug/EngineShaders` for the default Debug
configuration). Keep persistent engine shader sources under
`Source/Engine/GraphicsEngine/Shaders`; the build copies them into that folder.
The include supplies `Material_Vertex` and `Material_Pixel`. Unreal exports values
and texture references; computations live in these functions.

## Unreal parameters

Declare scalar and vector values in `MaterialBuffer : register(b4)`. The importer
adds `MB_` if absent and replaces spaces with underscores, for both master
materials and instances. For example, Unreal `Snow Amount` binds to
`float MB_Snow_Amount`, and `Tint Color` binds to `float4 MB_Tint_Color`.

Declare custom textures in available material slots (`t4` through `t15`). Standard
texture asset suffixes select fixed slots: `_BaseColor` selects t0, `_Normal` t1,
`_OcclusionRoughnessMetallic` t2, and `_Emissive` t3. For other assets, the shader
resource name must match the texture asset name exactly. For example, asset
`Noise_Map` binds to `Texture2D Noise_Map`. Exported texture parameter names do not
select resources. Renaming an asset requires updating the matching shader name;
there is no rename compatibility lookup. Reflection must retain the resource, so
sample it in the shader. Failed bindings log a warning.

See [Unreal-to-AGP material naming](UnrealAGPMaterialNaming.md) for input mappings
and the differences from Unreal's material interface.

## Pixel inputs and outputs

- `VertexColor`: interpolated mesh vertex color, available independently from
  `PixelColor`. The lit/deferred engine paths keep their existing albedo behavior;
  multiply by this field explicitly when desired.
- `UV0`, `UV1`: interpolated texture coordinates. Standard textures are sampled
  before `Material_Pixel`; sample textures explicitly inside the hook when using
  alternate or animated UVs, or modify UVs in `Material_Vertex`.
- `SurfaceValues.rgb`: sampled AO, roughness, metallic. Changes made in the hook
  feed both deferred and forward lighting. Lighting keeps its existing roughness
  clamp. Debug views show final surface values.
- `PixelColor`, `Normal`, `Emission`: existing editable material outputs. Color
  and world-normal debug views show the custom output. The local-normal view
  projects the final material normal into the mesh tangent frame, so blended or
  world-projected normal maps appear there too; it does not display only t1.
  Neutral tangent normals appear purple (RGB approximately 0.5, 0.5, 1).
  The emission debug view
  shows the raw red-channel FX texture mask as grayscale in both engine paths.

`FB_TotalTime` is elapsed scene simulation time in seconds, available in both
hooks through `Common.hlsli`. It advances with the world's sanitized update delta,
stops when updates stop, and resets when the world is cleared or replaced. Camera
and shadow draws receive the same time.

The older `Content/Shaders/Material` shader set is separate from the configured
engine shader set. Its pixel interface also exposes `VertexColor`, `UV1` and
`SurfaceValues`; this change does not migrate its lighting or frame-buffer layout.
