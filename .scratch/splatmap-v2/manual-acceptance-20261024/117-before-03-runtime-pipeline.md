# 03: Render and repeat the painted splatmap pipeline in AGP

Status: ready-for-agent
Blocked by: 02
Owner: unassigned
Spec: ../spec.md

Use the verified export in sibling engine branch SplatmapV2. Determine the
remaining pipeline steps from its real schema and assets. Keep source authoring
and reusable exports in AGP-Content; preview deployment before copying.

- [ ] Engine builds; relevant existing tests pass against the newly built code.
- [ ] Validated scene, geometry, textures and shader deploy with reviewed scope.
- [ ] Runtime renders the intended painted layer distribution and geometry;
  registration, axes/UVs, normals and ORM match the Unreal reference.
- [ ] Debug views distinguish raw mask channels from final layer contributions.
- [ ] A repaint, normal export and runtime reload show the expected change.
- [ ] New required engine changes are reviewed separately from the initial exact
  three-file port. Required checks and two-axis review have recorded evidence.

## Verification

Not run. Keep in-progress until the visual acceptance and repeat iteration pass.
