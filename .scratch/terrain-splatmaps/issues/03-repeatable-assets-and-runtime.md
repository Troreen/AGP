# 03: Repeatable assets and integrated verification scene

Status: in-progress
Blocked by: 01, 02
Owner: root

Spec: ../spec.md

Deliver content-owned preparation tooling and distinct runtime assets/scene.

Acceptance criteria:
- Mesh preparation retains yaw -90 while removing the unwanted UV reflection.
- Splat/noise DDS are linear, mipmapped and numerically verified.
- All nine existing surface maps and mask/noise roles reach the material.
- Coverage validation detects ground geometry outside the level mask.
- Runtime checks cover registration, metric repeat, rotated floaters, noise and
  final normal/ORM appearance; performance evidence is measured and documented.
- Existing source assets, dirty Unreal level and unrelated edits are preserved.

Verification: preparation checks, solution build, runtime scene, independent
spec/standards review. Keep ticket open while required visual evidence is missing.
