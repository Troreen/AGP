# A simple guide to materials and shaders

This guide explains where things go in our engine. You do not need to know
all the code before you start. Use an existing material as an example and
change one small thing at a time.

## Object, material, texture, shader

These are different things:

- **Object:** the shape in the scene, such as a sphere.
- **Material:** the appearance assigned to that shape.
- **Texture:** an image that supplies color, bumps, or a pattern.
- **Shader:** the small program that reads those images and decides how the surface looks.

**One material can use several textures.** Three images do not mean the object
needs three materials. For example:

```text
Sphere
  Snow material
    Packed snow texture
    Detail normal texture
    Sparkle texture
```

The shader combines those images. It can read the same image more than once,
for different purposes. You still assign that image only once.

## Where the files go

| What you are adding | Where it goes |
| --- | --- |
| Texture images, in DDS format | `Content/Textures/`, in a folder for your material |
| Your material's shader file | `Source/Engine/GraphicsEngine/Shaders/` |
| Material settings, texture assignments, and object assignments | The exported scene JSON file in `Content/ExportedScenes/` |

JSON is a text file containing named settings. Follow the layout of an existing
material in the [shader test scene](../Content/ExportedScenes/lvl_shader_testing_tarik/Lvl_Shader_Testing_Tarik_Level.json).

For exported scenes, the shader filename must match the **master material's
name**. A material called `MM_MySurface` needs `MM_MySurface.hlsli`.
Assign the material to the object's mesh in that scene.

## Where do we choose the images?

There are two connected parts. The shader gives the texture a name:

```hlsl
Texture2D PatternMap : register(t4);
```

Think of `t4` as a numbered place to plug in an image. Slots 0 to 3 already
have engine uses. Start extra textures at 4, use a different slot for each,
and stay below 16.

The actual DDS file is chosen in the material's `Parameters` list in the scene:

```json
{
  "Name": "PatternMap",
  "Type": 3,
  "Value": {
    "Name": "MyPattern",
    "Path": "/Game/Textures/MySurface/MyPattern.MyPattern",
    "AssetImportPath": "Content/Textures/MySurface/MyPattern.dds"
  }
}
```

`PatternMap` must match the shader's name. `Type: 3` means texture.
`AssetImportPath` points to the image file. The engine loads it and connects it
to the shader. Add another entry for each extra image in the same material.

## Reading an image

The material has a function called `Material_Pixel`. This is where you describe
the surface's appearance. Inside that function, an image read can look like:

```hlsl
float3 color = PatternMap.Sample(TrilinearWrap, parameters.UV0).rgb;
parameters.PixelColor.rgb = color;
```

In everyday terms: read the image, then use the result as the surface color.

- **UVs** tell the shader where to read the image. `UV0` comes from the model.
  Multiplying UVs by 3 repeats the image three times in each direction.
- **TrilinearWrap** smooths the image and lets it repeat.
- **`.rgb`** reads red, green, and blue together. **`.r`** reads just red.

An image can store different information in its red, green, blue, and alpha
channels. This is called **channel packing**. You must know what each channel
contains before using it. A normal map describes surface direction; a mask
usually uses black for none and white for full strength. Treat normal maps
and masks as linear data, without an sRGB color conversion.

## Choosing the result

Inside `Material_Pixel`, these are the main things you can change:

| Setting | Meaning |
| --- | --- |
| `parameters.PixelColor.rgb` | Surface color |
| `parameters.SurfaceValues.g` | Roughness: 0 is smooth, 1 is rough |
| `parameters.SurfaceValues.b` | Metallic: 0 is non-metal, 1 is metal |
| `parameters.Normal` | Surface direction, which changes how light catches it |
| `parameters.Emission.rgb` | Added brightness that does not need scene lighting |

Use a **lit** material for ordinary surface lighting. An **unlit** material
returns its color without that lighting; changing its roughness or emission
alone does not change its output in our current unlit shader.

Keep the other function, `Material_Vertex`, in your file too. You can leave its
body empty when you are only changing the surface appearance.

## Familiar Unreal tools

Our engine uses HLSL text instead of connected material nodes. These helpers
save you from writing some of the math yourself:

| Unreal idea | Our equivalent |
| --- | --- |
| Texture Sample | `.Sample(...)` |
| TexCoord | `parameters.UV0` |
| DeriveNormalZ, after multiplying XY by 2 and subtracting 1 | `Normal_DeriveZ(...)` |
| Shortcut for the whole red/green normal decoding step | `Normal_DecodeRG(...)` |
| BlendAngleCorrectedNormals | `Normal_BlendAngleCorrected(...)` |
| Turn a normal map's direction into a world direction | `Normal_TangentToWorld(...)` |
| Fresnel, for an edge or rim effect | `Fresnel_Schlick(...)` |
| Direction toward the camera | `View_DirectionToCamera(...)` |
| Atmosphere Sun Light Vector | `Sun_DirectionToLight()` |
| Repeating a texture by screen pixels | `UV_ScreenTexture(...)` |

Add `#include "Normal.hlsli"` for the normal helpers, or
`#include "MaterialUtilities.hlsli"` for the others. Normal maps start with
directions relative to the surface; our material's `Normal` output expects a
world direction. Decode and blend first, then convert. See the
[helper reference](MaterialShaderHelpers.md) when you need the exact inputs.

To keep the same steps as the Unreal graph, write them separately:

```hlsl
float2 normalXY = textureSample.rg * 2.0f - 1.0f;
float3 normalTS = normalize(Normal_DeriveZ(normalXY));
```

Here, `textureSample` means the result of reading your normal texture. The first
line changes the image values into signed directions. The second recovers the
missing Z direction; `normalize` keeps the complete direction one unit long.

## Try one change at a time

Start with a plain color, then try one texture. Add other effects once those
work. The existing `MM_BrownRock08.hlsli` shows the shape of a material file,
but you do not need to copy all its behavior.

Build `Game.sln` in Debug x64 to copy shader files to the runtime folder, then
restart the game. Check the result close up, far away, and while moving the
camera. If something looks wrong, temporarily show just the texture or mask
as the surface color to see what you are reading.
