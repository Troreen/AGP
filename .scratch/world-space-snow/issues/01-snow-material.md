# 01: Snow on shader-test material

Status: in-progress
Blocked by: none
Owner: snow_implementation

## What to build

Implement [the spec](../spec.md) through the existing named master-material shader path. Code pointer: Source/Engine/GraphicsEngine/Shaders and SceneWorldBuilder::CreateSceneMaterials. Base: d627d712a1024b4cfb7f1b623692e51ef4714ae9.

## Acceptance criteria

- [ ] MM_ShaderTest automatically selects the new shader.
- [ ] Position-derived world-up coverage blends tops to white and preserves bare sides/undersides.
- [ ] Coverage responds to object rotation, is translation independent, and is camera independent.
- [ ] Base color/alpha and lighting remain functional; other materials are unchanged.
- [ ] Build and material program compilation pass; required runtime visual checks pass.

## Verification

Pending. Follow the scene, controls, transforms, and expected results in the spec. Do not stage or commit. No project membership changes are needed for runtime shader assets.

## Comments

Single vertical slice; no dependencies. Keep in-progress until required visual evidence is available.
