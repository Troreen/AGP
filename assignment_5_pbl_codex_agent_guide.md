# Codex Agent Guide — Assignment 5: Physically Based Rendering / Lighting

This file is for a Codex-style coding agent. Keep it implementation-focused. Do not rewrite the engine. Extend existing systems and preserve all previous assignment requirements.

Source lecture: **F11 / F10 — Physically Based Lighting**.

---

## Goal

Replace the old **Lambertian Reflectance** lighting in the `Lit` shading model with **Physically Based Lighting (PBL)**.

After this assignment:

- `Lit` uses PBL.
- `Unlit` still works.
- Lambertian Reflectance is no longer used for final lighting.
- Existing light attenuation may stay:
  - inverse-square falloff
  - range attenuation
  - spot cone attenuation
- Shadow mapping must still work.
- Shadows affect direct light only.
- Ambient / IBL should still light shadowed areas.

---

## Mandatory First Step: Audit Existing Code

Before implementing, inspect the codebase and determine what is complete, partial, or missing.

Check:

- Material system
- Lit / Unlit shader selection
- Albedo texture support
- Normal texture support
- Shadow mapping
- Directional / point / spot lights
- Light buffer
- Texture creation support
- Cube map loading/support
- Sampler creation/support
- Shader resource binding
- Material JSON / `.mat` format
- Existing HLSL lighting functions

Do not assume a system is complete because a class or file exists.

---

## Assignment Requirements

### Existing features that must still work

- Static FBX model loading
- Skeletal FBX model loading
- Animation playback
- Numpad 1 / 2 / 3 animation switching
- Simple materials
- Textures
- Normals and normal maps
- `Lit` shading model
- `Unlit` shading model
- Simple shadow mapping
- Directional, point, and spot lights
- Numpad 7 / 8 / 9 light toggles
- Camera controls
- At least two 3D primitives visible
- A skeletal mesh playing an animation
- All meshes standing on a plane for shadow verification

### New Assignment 5 requirements

- Add **Physically Based Lighting**.
- Replace Lambertian Reflectance in the `Lit` shading model.
- Add **Material Texture** support.
- Materials must use:
  - Albedo texture
  - Normal texture
  - Material texture
- Scene must be lit using PBL.
- Shadows must still be cast from lights.
- Shadows must be lit by the ambient component from PBL.

---

## Material Texture

Every material now needs three textures:

```hlsl
Texture2D AlbedoTexture   : register(t0);
Texture2D NormalTexture   : register(t1);
Texture2D MaterialTexture : register(t2);
```

Recommended global textures:

```hlsl
Texture2D BRDF_LUT_Texture : register(t99);
TextureCube EnvCubeTexture : register(/* choose a global slot below shadow maps */);
```

Keep material textures below `t16`. Keep shadow maps at the existing high slots, such as `t100+`.

---

## Material Texture Channel Packing

Use the course/Epic-style packing:

```text
Material texture:
R = Ambient Occlusion
G = Roughness
B = Metalness
A = unused
```

Shader sample:

```hlsl
float3 materialMap = MaterialTexture.Sample(TrilinearWrap, input.UV0).rgb;

float ao        = materialMap.r;
float roughness = materialMap.g;
float metalness = materialMap.b;
```

Use default textures if a material is missing one.

---

## PBL Surface Inputs

The Lit shader needs:

```text
albedo
normal
ambient occlusion
roughness
metalness
view direction
light direction
light color/intensity
shadow factor
environment cube map
BRDF LUT
```

All lighting calculations must happen in **world space**.

---

## Precomputed Colors

Prepare diffuse/specular colors before light calculations:

```hlsl
float3 diffuseColor =
    lerp((float3)0.0f, albedo, 1.0f - metalness);

float3 specularColor =
    lerp((float3)0.04f, albedo, metalness);
```

Meaning:

- Metals have colored specular and almost no diffuse.
- Non-metals have diffuse color and neutral specular around `0.04`.

---

## Required HLSL BRDF Functions

Create separate functions. Do not bury the math inline.

```hlsl
float3 Diffuse_BRDF(float3 diffuseColor);

float NormalDistributionFunction_GGX(
    float roughness,
    float3 normal,
    float3 halfVector);

float3 Fresnel_SphericalGaussianSchlick(
    float3 specularColor,
    float3 viewDir,
    float3 halfVector);

float GeometricAttenuation_Schlick_GGX(
    float roughness,
    float3 normal,
    float3 lightDir,
    float3 viewDir);

float3 Specular_BRDF(
    float roughness,
    float3 normal,
    float3 halfVector,
    float3 viewDir,
    float3 lightDir,
    float3 specularColor);

float3 CalculateDiffuseIBL(...);
float3 CalculateSpecularIBL(...);
```

---

## Core BRDF Formula Snippets

Define:

```hlsl
static const float PI = 3.14159265f;
```

Diffuse:

```hlsl
float3 Diffuse_BRDF(float3 diffuseColor)
{
    return diffuseColor / PI;
}
```

GGX normal distribution:

```hlsl
float NormalDistributionFunction_GGX(float roughness, float3 normal, float3 h)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH = saturate(dot(normal, h));
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (alpha2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;
    return alpha2 / max(denom, 0.00001f);
}
```

Fresnel approximation:

```hlsl
float3 Fresnel_SphericalGaussianSchlick(float3 specularColor, float3 viewDir, float3 h)
{
    float VdotH = saturate(dot(viewDir, h));
    float power = (-5.55473f * VdotH - 6.98316f) * VdotH;
    return specularColor + (1.0f - specularColor) * exp2(power);
}
```

Geometric attenuation:

```hlsl
float GeometricAttenuation_Schlick_GGX_G1(float roughness, float NdotX)
{
    float k = ((roughness + 1.0f) * (roughness + 1.0f)) / 8.0f;
    return NdotX / max(NdotX * (1.0f - k) + k, 0.00001f);
}

float GeometricAttenuation_Schlick_GGX(float roughness, float3 normal, float3 lightDir, float3 viewDir)
{
    float NdotL = saturate(dot(normal, lightDir));
    float NdotV = saturate(dot(normal, viewDir));
    return GeometricAttenuation_Schlick_GGX_G1(roughness, NdotL)
         * GeometricAttenuation_Schlick_GGX_G1(roughness, NdotV);
}
```

Specular:

```hlsl
float3 Specular_BRDF(
    float roughness,
    float3 normal,
    float3 h,
    float3 viewDir,
    float3 lightDir,
    float3 specularColor)
{
    float D = NormalDistributionFunction_GGX(roughness, normal, h);
    float3 F = Fresnel_SphericalGaussianSchlick(specularColor, viewDir, h);
    float G = GeometricAttenuation_Schlick_GGX(roughness, normal, lightDir, viewDir);

    float NdotL = saturate(dot(normal, lightDir));
    float NdotV = saturate(dot(normal, viewDir));
    float denom = max(4.0f * NdotL * NdotV, 0.00001f);

    return (D * F * G) / denom;
}
```

---

## Direct Light Functions

Update existing light functions to call the BRDF functions:

```hlsl
CalculateDirectionalLight(...)
CalculatePointLight(...)
CalculateSpotLight(...)
```

Each must calculate PBL direct lighting.

Keep existing attenuation logic:

- Directional light: no distance attenuation.
- Point light: inverse-square + range attenuation.
- Spot light: inverse-square + range attenuation + cone attenuation.

Do **not** mix in old Lambert code.

General direct-light structure:

```hlsl
float3 h = normalize(lightDir + viewDir);

float3 kS = Specular_BRDF(
    roughness,
    normal,
    h,
    viewDir,
    lightDir,
    specularColor);

float3 kD = Diffuse_BRDF(diffuseColor);
kD *= (1.0f - kS);

float NdotL = saturate(dot(normal, lightDir));

float3 direct =
    (kD + kS) *
    lightColorAndIntensity *
    NdotL *
    attenuation *
    shadow;
```

Use the existing project convention for light direction signs.

---

## Environment Map / IBL

Add or verify Environment Cube Map support.

The environment map is used for ambient / indirect lighting.

Use `TextureCube`. Sampling requires a `float3` direction.

---

## BRDF LUT Texture

The BRDF LUT is a global GraphicsEngine-owned texture.

Create it once during initialization.

Recommended settings:

```text
Size: 512x512
Format: DXGI_FORMAT_R16G16_FLOAT
Bind flags: SHADER_RESOURCE | RENDER_TARGET
```

Bind it to the pixel shader at the start of each render frame, for example:

```text
t99
```

---

## LUT Creation

Add support for creating renderable textures if missing.

Create a one-time PSO:

```text
Vertex Shader: FullTexture_VS
Pixel Shader: BRDF_LUT_PS
Topology: Triangle Strip
```

Add `TriangleStrip` to the topology enum if missing. Lecture says D3D value is `5`.

Implementation steps:

1. Create LUT texture.
2. Create temporary/init command list.
3. Set LUT PSO.
4. Set LUT texture as render target without depth.
5. Call `Draw(4, 0)`.
6. Execute.
7. Keep only the LUT texture.

---

## FullTexture_VS

Use `SV_VertexID`; no vertex/index buffers needed.

```hlsl
FullTextureVertex main(uint vertexID : SV_VertexID)
{
    const float4 pos[4] =
    {
        float4(-1, -1, 0, 1),
        float4(-1,  1, 0, 1),
        float4( 1, -1, 0, 1),
        float4( 1,  1, 0, 1)
    };

    const float2 uv[4] =
    {
        float2(0, 1),
        float2(0, 0),
        float2(1, 1),
        float2(1, 0)
    };

    FullTextureVertex output;
    output.Position = pos[vertexID];
    output.UV = uv[vertexID];
    return output;
}
```

---

## BRDF LUT Pixel Shader

Use the provided `IntegrateBRDF` function.

```hlsl
float2 main(FullTextureVertex input) : SV_TARGET
{
    return IntegrateBRDF(input.UV.x, input.UV.y);
}
```

---

## LUT Sampler

Create a special sampler for the LUT:

```text
Linear Clamp
```

Do not use wrap for LUT sampling.

---

## IBL Functions

Get mip count:

```hlsl
int GetNumMips(TextureCube cubeMap)
{
    int w = 0;
    int h = 0;
    int m = 0;
    cubeMap.GetDimensions(0, w, h, m);
    return m;
}
```

Diffuse IBL:

```hlsl
float3 CalculateDiffuseIBL(float3 pixelNormal, TextureCube envCube)
{
    int numMips = GetNumMips(envCube) - 1;
    return envCube.SampleLevel(TrilinearWrap, pixelNormal, numMips).rgb;
}
```

Specular IBL:

```hlsl
float3 CalculateSpecularIBL(
    float3 specularColor,
    float3 pixelNormal,
    float3 viewDir,
    float roughness,
    TextureCube envCube)
{
    int numMips = GetNumMips(envCube) - 1;
    float3 R = reflect(-viewDir, pixelNormal);

    float3 envColor = envCube.SampleLevel(
        TrilinearWrap,
        R,
        roughness * numMips).rgb;

    float NdotV = saturate(dot(pixelNormal, viewDir));

    float2 brdfLUT = BRDF_LUT_Texture.Sample(
        LUTSampler,
        float2(NdotV, roughness)).rg;

    return envColor * (specularColor * brdfLUT.x + brdfLUT.y);
}
```

Ambient / indirect lighting:

```hlsl
float3 diffuseIBL = CalculateDiffuseIBL(pixelNormal, EnvCubeTexture);
float3 specularIBL = CalculateSpecularIBL(specularColor, pixelNormal, viewDir, roughness, EnvCubeTexture);
float3 ambient = (diffuseColor * diffuseIBL + specularIBL) * ao;
```

Important:

```text
Shadows must not affect ambient.
```

---

## Lit Pixel Shader Flow

The final Lit shader should roughly follow this order:

1. Sample albedo texture.
2. Sample and unpack normal map.
3. Transform normal to world space using TBN.
4. Sample material texture:
   - AO
   - Roughness
   - Metalness
5. Calculate `diffuseColor`.
6. Calculate `specularColor`.
7. Calculate world-space `viewDir`.
8. Calculate ambient/IBL.
9. Loop over lights.
10. For each light:
    - calculate direct PBL light
    - apply attenuation
    - apply shadow factor
11. Final color:

```hlsl
float3 finalColor = ambient + directLightSum;
```

12. Gamma-correct only at the end.
13. Return final color.

---

## Shadow Interaction

Old shadow maps should still be used.

Change their role:

```text
directLight *= shadow;
ambient remains unchanged.
```

Do not do:

```hlsl
finalColor *= shadow;
```

Correct:

```hlsl
float3 finalColor = ambient + directLightingWithShadows;
```

---

## Gamma Correction

Do all lighting in linear space.

Gamma-correct only final output.

```hlsl
float3 LinearToGamma(float3 color)
{
    return pow(abs(color), 1.0f / 2.2f);
}

return float4(LinearToGamma(finalColor), 1);
```

---

## Render Flow

Recommended render flow:

1. Reset pipeline.
2. Render/update shadow maps as before.
3. Set main camera framebuffer.
4. Set samplers:
   - Trilinear Wrap
   - Shadow comparison
   - LUT Linear Clamp
5. Bind global PBL resources:
   - Environment Cube Map
   - BRDF LUT texture
   - Shadow maps
6. Collect lights and fill LightBuffer.
7. Render meshes.
8. For each mesh:
   - Bind object buffer
   - Bind animation buffer if needed
   - Bind material
   - Bind albedo texture to `t0`
   - Bind normal texture to `t1`
   - Bind material texture to `t2`
   - Render using `Lit` or `Unlit`

---

## Debugging Steps

### Material Texture

```hlsl
return float4(ao, roughness, metalness, 1);
```

Expected:

```text
R = AO
G = Roughness
B = Metalness
```

### Direct PBL only

Temporarily disable ambient:

```hlsl
float3 ambient = 0;
```

Verify directional, point, spot, attenuation, and shadows.

### IBL only

Temporarily disable direct lights.

Verify environment map sampling, diffuse IBL, specular IBL, roughness mip selection, BRDF LUT sampling, and AO.

### Full PBL

Enable both:

```hlsl
finalColor = ambient + direct;
```

---

## Common Mistakes

- Keeping old Lambert code in `Lit`.
- Applying shadow to ambient.
- Gamma-correcting before lighting.
- Forgetting to bind Material Texture.
- Swapping roughness and metalness channels.
- Using wrap sampler for LUT.
- Forgetting to bind BRDF LUT every frame.
- Forgetting to bind Environment Cube Map.
- Using view/local-space vectors instead of world-space vectors.
- Forgetting that metals should have no diffuse.
- Forgetting that roughness should only be remapped inside the geometry function.
- Shadow maps becoming faint because direct light intensity is too low after PBL.

---

## Final Checklist

### Materials

- [ ] Materials support Albedo texture.
- [ ] Materials support Normal texture.
- [ ] Materials support Material texture.
- [ ] Missing textures use defaults.
- [ ] Material texture channel packing is correct.

### Graphics Engine

- [ ] Can create renderable textures.
- [ ] Creates BRDF LUT texture at startup.
- [ ] Renders BRDF LUT once.
- [ ] Owns BRDF LUT texture.
- [ ] Loads/binds Environment Cube Map.
- [ ] Creates LUT Linear Clamp sampler.
- [ ] Adds Triangle Strip topology.

### HLSL

- [ ] `Diffuse_BRDF` exists.
- [ ] `Specular_BRDF` exists.
- [ ] `NormalDistributionFunction_GGX` exists.
- [ ] `Fresnel_SphericalGaussianSchlick` exists.
- [ ] `GeometricAttenuation_Schlick_GGX` exists.
- [ ] `CalculateDiffuseIBL` exists.
- [ ] `CalculateSpecularIBL` exists.
- [ ] Directional light uses PBL.
- [ ] Point light uses PBL.
- [ ] Spot light uses PBL.
- [ ] Lambert is not used by Lit.
- [ ] Unlit still works.

### Shadows

- [ ] Shadows still render.
- [ ] Shadows affect direct light.
- [ ] Ambient/IBL remains visible in shadow.
- [ ] Shadow maps are not broken by PBL changes.

### Startup Scene

- [ ] At least two 3D primitives visible.
- [ ] Controllable camera.
- [ ] Animated skeletal mesh renders correctly.
- [ ] Each mesh has its own material.
- [ ] Directional, point, and spot lights exist.
- [ ] All meshes stand on a plane.
- [ ] Materials use Albedo, Normal, and Material textures.
- [ ] Scene is lit with PBL.
- [ ] Shadows are cast from lights.

---

## Completion Definition

Assignment 5 is complete when:

- The `Lit` shading model uses PBL instead of Lambert.
- `Unlit` still works.
- Every material uses Albedo, Normal, and Material textures.
- Directional, Point, and Spot Lights produce PBL lighting.
- Environment IBL and BRDF LUT contribute ambient lighting.
- Shadows still work and only affect direct lighting.
- Shadowed areas still receive ambient/IBL.
- All previous assignment requirements still pass.
