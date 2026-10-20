# Unreal-to-AGP material and shader naming

Use Unreal's material terms when describing material behavior. The table records
the existing AGP HLSL interface; it does not introduce aliases or rename shader
fields. It applies to `Source/Engine/GraphicsEngine/Shaders`, the configured engine
shader set. `Content/Shaders/Material` is an older, separate set.

## Material outputs and mesh inputs

| Unreal term | AGP HLSL | Meaning and differences |
| --- | --- | --- |
| Base Color | `MaterialPixelParameters.PixelColor.rgb` | Material color before lighting. Existing AGP parameters also use `MB_DiffuseColor`. |
| Ambient Occlusion | `MaterialPixelParameters.SurfaceValues.r` | Editable material AO, conventionally 0 to 1. This is separate from screen-space AO. |
| Roughness | `MaterialPixelParameters.SurfaceValues.g` | Editable roughness; engine lighting clamps it to 0.04 through 1. |
| Metallic | `MaterialPixelParameters.SurfaceValues.b` | Editable metallic value, conventionally 0 to 1. Older AGP names use `Metalness` / `MB_Metalness`. |
| Emissive Color | `MaterialPixelParameters.Emission.rgb` | Editable RGB emission. The initial value is the FX texture's red mask multiplied by sampled albedo. The raw mask remains the emission debug preview. |
| Normal | `MaterialPixelParameters.Normal` | AGP's hook receives and edits a world-space normal after normal-map conversion. Unreal materials normally use tangent-space normals unless configured otherwise. |
| VertexNormalWS | `MaterialPixelParameters.VertexNormal` | Interpolated world-space mesh normal before normal mapping. The AGP field is available in the pixel hook; Unreal's named expression has shader-stage restrictions. |
| VertexColor | `MaterialPixelParameters.VertexColor` | Interpolated vertex color, separate from `PixelColor`. |
| TextureCoordinate, Coordinate Index 0 | `MaterialPixelParameters.UV0` | First UV set. |
| TextureCoordinate, Coordinate Index 1 | `MaterialPixelParameters.UV1` | Second UV set. |
| WorldPosition | `MaterialPixelParameters.WorldPosition.xyz` | World position. Unreal uses Z-up; AGP uses Y-up, so directional masks need the corresponding axis. |
| Time | `FB_TotalTime` | Elapsed scene simulation seconds, using sanitized/clamped world delta. Resets on world clear/replacement. This is not a promise of Unreal's time origin or Ignore Pause behavior. |

`SurfaceValues.rgb` packs Ambient Occlusion, Roughness, Metallic in that order.
`Material_Pixel` can compute these values from constants, textures or expressions;
forward lighting and the GBuffer consume its modified values. This provides
ordinary Unreal-style material controls. It does not let a material replace the
renderer SSAO pass, shadow system or lighting model. AGP combines material AO with
SSAO for ambient lighting; this is not a claim of identical Unreal rendering.

Standard textures are sampled before `Material_Pixel`. To use UVs computed inside
the hook, explicitly sample the texture with those UVs. Changing a UV field alone
does not repeat an earlier sample. UV changes in `Material_Vertex` feed the later
standard texture samples.

## Imported parameters

| Unreal/export name | AGP name or binding rule |
| --- | --- |
| Scalar or vector parameter `Snow Amount` | `MB_Snow_Amount` in `MaterialBuffer : register(b4)`. Spaces become underscores; `MB_` is added if absent. Master and instance parameters use the same rule. |
| Existing parameter `MB_Snow_Amount` | Remains `MB_Snow_Amount`. |
| Exported base property `Base Color` | `MB_DiffuseColor`, configured in `Bin/Settings/MaterialNameConversions.json`. |
| Exported base property `Metallic` | `MB_Metalness`, configured in the same file. |
| Exported base property `Roughness` | `MB_Roughness`, configured in the same file. |
| Texture asset ending `_BaseColor` | `AlbedoTexture`, slot t0. |
| Texture asset ending `_Normal` | `NormalTexture`, slot t1. |
| Texture asset ending `_OcclusionRoughnessMetallic` | `MaterialTexture`, slot t2. |
| Texture asset ending `_Emissive` | `FxTexture`, slot t3; emission mask is red only. |
| Other texture asset, for example `Noise_Map` | Exact shader resource name `Noise_Map`, normally in t4 through t15. |

Unreal identifies texture parameters by their parameter names. AGP currently uses
the asset naming rules above; exported texture parameter names do not override
them. Assets and matching custom shader resources must be maintained together.
No automatic asset-rename support is provided.

Scalar/vector names remain subject to the existing HLSL naming convention; this
is why Unreal labels containing spaces cannot be used verbatim. New descriptive
names should use Unreal terms such as `BaseColor`, `Metallic`, `EmissiveColor` and
`AmbientOcclusion`. Existing fields and parameters are documented here to avoid
breaking current material includes.

## Scope

This addition exposes material surface controls, vertex color, UV1 and simulation
time. It does not add new shading models, opacity masking, vertex displacement
support or Unreal material-graph translation. Individual `.hlsli` material files
remain responsible for computations; exported parameters supply their values.

## Unreal references

- [Material Inputs](https://dev.epicgames.com/documentation/en-us/unreal-engine/material-inputs-in-unreal-engine)
- [Coordinates Material Expressions](https://dev.epicgames.com/documentation/en-us/unreal-engine/coordinates-material-expressions-in-unreal-engine)
- [Constant Material Expressions](https://dev.epicgames.com/documentation/en-us/unreal-engine/constant-material-expressions-in-unreal-engine)
- [Material Parameter Expressions](https://dev.epicgames.com/documentation/en-us/unreal-engine/material-parameter-expressions-in-unreal-engine)
