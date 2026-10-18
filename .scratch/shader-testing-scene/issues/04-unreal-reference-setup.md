# 04: Create Unreal reference setup
Status: ready-for-human
Blocked by: none

Spec: ../spec.md

## What to build
In Unreal add gray floor, matte gray sphere and polished metal sphere; use directional intensity 2 and export. The local exported JSON already uses 2; synchronize that value back into Unreal when available. External project location is pending. Do not substitute code spawning.

## Acceptance criteria
- [ ] Floor and two reference spheres authored and exported from Unreal.
- [x] Local exported directional intensity 2 visually accepted with the existing bloom enabled.
- [ ] Directional intensity 2 carried back into the Unreal source scene.
- [ ] Reference materials, layout and final light balance visually accepted.

## Verification
Inspect source/export and runtime placement/materials. User visual acceptance required.

## Comments
Owner: human
