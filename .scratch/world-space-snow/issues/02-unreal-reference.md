# 02: Reproduce Unreal snow graph in MM_Standard_WithSnow

Status: in-progress
Blocked by: none
Owner: root

Spec: [unreal-reference-spec.md](../unreal-reference-spec.md).
Reference: Docs/ShaderUnreal.png.
Base: d627d712a1024b4cfb7f1b623692e51ef4714ae9; branch world-space-snow.

## Acceptance criteria

- [ ] Exact material name automatically selects the new shader.
- [ ] Color is lerp(0.2, 0.9, saturate(worldPixelNormal.y * 4.5 - 2.5)).
- [ ] Normal map affects coverage; Flatness defaults to 0 and 1 flattens normal detail.
- [ ] Existing unrelated materials retain their behavior, and shader compilation/build pass.
- [ ] Runtime comparison with the reference passes.

## Verification

Pending. Do not stage or commit. Preserve concurrent user changes to settings, exported scene, project membership, and reference image.
