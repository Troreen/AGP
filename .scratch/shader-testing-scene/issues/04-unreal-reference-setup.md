# 04: Create Unreal reference setup
Status: ready-for-human
Blocked by: none

Spec: ../spec.md

## What to build
In Unreal add gray floor, matte gray sphere and polished metal sphere; use directional intensity 2 and export. The local exported JSON already uses 2; synchronize that value back into Unreal when available. External project location is pending. Do not substitute code spawning.

## Acceptance criteria
- [x] Plain gray 20 m ground added directly to the exported JSON using SM_Plane, as subsequently authorized.
- [ ] Two reference spheres authored and exported from Unreal.
- [x] Local exported directional intensity 2 visually accepted with the existing bloom enabled.
- [ ] Directional intensity 2 carried back into the Unreal source scene.
- [x] Final light/bloom balance accepted at intensity 2.
- [x] User accepted the corrected gray ground size and placement.
- [ ] Reference-sphere materials/layout visually accepted.

## Verification
Inspect source/export and runtime placement/materials. User visual acceptance required.

## Comments
Owner: human
