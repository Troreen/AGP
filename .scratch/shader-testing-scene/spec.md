# Shader-testing scene improvements
Status: in-progress

## Problem Statement
The shader-testing scene has no useful environment reflections/background and its directional light feels too strong. Manual characters clutter testing.

## Solution
Use the existing snowy hillside HDRI as a fixed environment and skybox in the shader scene and export-and-play. Keep the current mip-based approximation and tonemapper. Author a gray floor and matte gray/polished metal reference spheres in Unreal. Use directional-light intensity 2, accepted by the user after comparison with bloom off and on. The local exported scene is updated; carry this value back into Unreal when source access is available.

## Implementation Decisions
- Comment out complete Player, Enemy, and optional A_Debug setup blocks globally; preserve the code.
- Convert the source HDRI offline into an HDR-capable DDS cubemap with ordinary mip levels. Preserve the source.
- Keep environment strength unchanged. No new controls or exporter schema changes.
- Use the same cubemap orientation for background and reflections; camera translation must not move the skybox.
- Preserve geometry depth and exclude skybox from material debug views.
- The named shader scene and Content/EAP.json opt into the hillside. Other named scenes restore the default white environment. EAP-first startup remains unchanged; user explicitly requested deleting the current generated EAP.json, which can be regenerated later. Failed loads preserve the active environment.
- Mesh selection and directional-light settings remain authored in Unreal.

## Testing Decisions
Use the existing executable/scene, camera and debug-view controls. Build Debug and Release. Check startup/F4 absence of manual characters, reflections, roughness, orientation, translation, geometry occlusion, and scene isolation. Final visual acceptance belongs to the user; do not resolve tickets awaiting that evidence. No invented headless renderer or implementation-mirroring tests.

## Out of Scope
Prepared diffuse/specular IBL, native Sky Light export, ImGui, runtime selectors, exposure changes, presets, and tonemapper changes.

## Future Todos
Prepared IBL; ImGui environment selection/rotation/intensity; ImGui mesh selection; exposure controls and lighting presets.
