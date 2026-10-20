# 01: Custom surface outputs and mesh inputs
Status: in-progress
Blocked by: none
Owner: surface-inputs

Spec: ../spec.md

## Acceptance criteria
- Forward/deferred consume modified SurfaceValues after the hook; debug views reflect final material values.
- Pixel hook receives independent VertexColor and UV1; all callers initialize fields.
- Existing individual material includes remain compatible; relevant legacy content interfaces compile.

## Verification
Compile shaders/build; inspect shader test scene for metallic/roughness changes and vertex/UV masks. Record actual evidence and limitations.

## Implementation
- Engine forward lighting reads AO/roughness/metalness after Material_Pixel; GBuffer stores the modified packed surface values.
- Forward debug albedo/normal use final material outputs. User correction (2026-10-20): forward/deferred emission previews retain the raw red-channel mask as grayscale.
- Added independent VertexColor and UV1 to engine pixel parameters and initialized all three callers. Legacy Content callers now initialize VertexColor and SurfaceValues and consume modified surface values.

## Evidence
- `powershell -NoProfile -ExecutionPolicy Bypass -File .scratch/custom-shader-inputs/verify-material-shaders.ps1`: passed 30 fxc Shader Model 5 compilations. Covers default and custom VertexColor/UV1/SurfaceValues fixtures through forward, deferred, unlit and vertex stages in both trees; both existing engine material includes; legacy _DefaultMaterial GBuffer/vertex; all four engine deferred lighting shaders.
- Initial direct .ps1 invocation was blocked by local PowerShell execution policy; process-only Bypass ran successfully.
- Repeated the matrix successfully after moving generated shader copies and binaries to ignored `Intermediate/CustomShaderVerification`.
- Existing Content/_DefaultMaterial Lit/Unlit fails with X4578 because its b4 MaterialBuffer overlaps legacy Lighting's b4 LightBuffer. Those two pre-existing legacy combinations are excluded; no renderer migration attempted.
- No image assertions or runtime visuals performed by ticket owner. Compilation verifies the public hook interface and existing include compatibility, not rendered behavior. Root owns combined build/review/runtime verification.
- A standalone WARP readback experiment was stopped during compilation and its three new source/project/fixture files removed after standards review identified the workflow's explicit ban on inventing a headless renderer. No readback test or executable ran; this is not acceptance evidence.

## Pending visual acceptance
Run the material shader test scene in forward and deferred modes with identical lights. Adjust imported Roughness and Metalness on MM_ShaderTest; debug roughness/metalness must change accordingly and agree across paths. Use the generated custom fixture with a mesh containing distinct vertex colors and UV1: color must show their product, AO=0.25, roughness=0.65, metallic=0.85 in debug views. Existing material includes must keep their previous defaults. Keep this ticket in-progress until observed.
