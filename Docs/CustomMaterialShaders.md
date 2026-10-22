# Custom material shaders

Each exported Unreal master material uses an individual `<MasterMaterialName>.hlsli`
in the configured shader folder (`Bin/Debug/EngineShaders` for the default Debug
configuration). Keep engine shader sources under
`Source/Engine/GraphicsEngine/Shaders` and game materials in AGP-Content's
`Content/Shaders`; copy content into AGP before building. The build deploys
content shaders first and engine shaders second, so engine files win collisions.
The include supplies `Material_Vertex` and `Material_Pixel`. Unreal exports values
and texture references; computations live in these functions.

## Unreal parameters

Declare scalar and vector values in `MaterialBuffer : register(b4)`. The importer
adds `MB_` if absent and replaces spaces with underscores, for both master
materials and instances. For example, Unreal `Snow Amount` binds to
`float MB_Snow_Amount`, and `Tint Color` binds to `float4 MB_Tint_Color`.

Declare custom textures in available material slots (`t4` through `t15`). The
exported texture parameter name selects a matching reflected resource first;
spaces become underscores and texture names do not acquire an `MB_` prefix.
For example, parameter `Rough BaseColor` selects `Texture2D Rough_BaseColor`
regardless of its assigned asset's suffix. Resource matching is case-insensitive.
If no matching parameter resource exists, legacy texture asset suffixes select
fixed slots: `_BaseColor` selects t0, `_Normal` t1,
`_OcclusionRoughnessMetallic` t2, and `_Emissive` t3. Other assets fall back to a
resource matching the asset name, such as `Noise_Map`. Reflection must retain
the resource, so sample it in the shader. Failed bindings log a warning.

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
  and world-normal debug views show the custom output. The emission debug view
  shows the raw red-channel FX texture mask as grayscale in both engine paths.

`FB_TotalTime` is elapsed scene simulation time in seconds, available in both
hooks through `Common.hlsli`. It advances with the world's sanitized update delta,
stops when updates stop, and resets when the world is cleared or replaced. Camera
and shadow draws receive the same time.

The older `Content/Shaders/Material` shader set is separate from the configured
engine shader set. Its pixel interface also exposes `VertexColor`, `UV1` and
`SurfaceValues`; this change does not migrate its lighting or frame-buffer layout.
