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

Declare custom textures in available material slots (`t4` through `t15`). Exported
texture parameter `Noise Map` binds to a declared `Texture2D Noise_Map`, regardless
of the texture asset name. Texture names use space replacement without adding
`MB_`. Reflection must retain the resource, so sample it in the shader.
If no exported parameter-name binding exists, the importer preserves standard
asset suffix mapping (`_BaseColor`, `_Normal`, `_OcclusionRoughnessMetallic`,
`_Emissive`) and then legacy resource lookup by asset name. Failed bindings log a
warning with the material and parameter names.

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
- `PixelColor`, `Normal`, `Emission`: existing editable material outputs. Color,
  world-normal and emission debug views now show the custom output.

`FB_TotalTime` is elapsed scene simulation time in seconds, available in both
hooks through `Common.hlsli`. It advances with the world's sanitized update delta,
stops when updates stop, and resets when the world is cleared or replaced. Camera
and shadow draws receive the same time.

The older `Content/Shaders/Material` shader set is separate from the configured
engine shader set. Its pixel interface also exposes `VertexColor`, `UV1` and
`SurfaceValues`; this change does not migrate its lighting or frame-buffer layout.
