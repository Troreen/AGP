# Unreal reference setup and visual acceptance

Status: pending source-scene access and user review.

The Unreal project/exporter is outside this checkout. The current exported shader scene still contains its original single test mesh and directional-light intensity 2 (locally tuned and accepted). The engine changes do not override this authoring value or inject reference geometry.

## Author in Unreal
1. Open the source shader-testing level corresponding to `lvl_shader_testing_tarik/Lvl_Shader_Testing_Tarik_Level.json`.
2. Keep the existing test mesh/material. Add a matte gray floor and two separate reference spheres: matte gray dielectric and polished metal. Keep them beside the test mesh without overlap; leave room for camera inspection. Reference materials must use the standard material path, not the snow shader.
3. Set the Unreal directional light intensity to 2 to match the locally tuned and accepted export. Preserve authoring of direction/color in Unreal.
4. Export the scene and required meshes/materials through the normal exporter. No Sky Light data or environment component is needed; the engine selects the fixed hillside for this scene path.
5. Reload with F4 and review positions, shadows, brightness, reflections and reference materials with the user. Final values remain subject to that visual review.

## Runtime acceptance
- Run Bin/Debug/Game.exe with the shader scene selected in the existing settings.
- Confirm no manually spawned Player, Enemy or A_Debug actor appears, including after F4.
- Rotate camera: hillside stays aligned with world-space reflections. Translate camera: background has no translation parallax.
- Inspect opaque edges and transparent geometry where available: background must stay behind meshes and transparency must retain depth behavior.
- Inspect matte and polished spheres and compare roughness responses (ordinary-mip approximation only).
- Use F5/F6 to inspect albedo, roughness, metalness and normals: no photographic background should contaminate those views.
- Reload with F4 repeatedly. Confirm the same environment returns without stale state.
- Launch another exported scene through the existing settings and confirm its old environment/lighting; manual-character removal is intentionally global. Restore the user's settings after this check.
- Exercise a failed later scene request through the existing request API if available; active world/environment should remain unchanged. Do not claim this runtime check from source inspection alone.
- User accepts the final directional/environment balance before task 05 is resolved.

## Startup scope update
The user requested keeping EAP-first startup, including future EAP exports in the fixed hillside environment, and deleting the current generated Content/EAP.json. That deletion is implemented. A new launch now selects the configured named shader scene. Other named scenes retain their default environment.
