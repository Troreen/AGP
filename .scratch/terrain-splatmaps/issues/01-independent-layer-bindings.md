# 01: Independent layer bindings and faithful material packing

Status: in-progress
Blocked by: none
Owner: engine_materials

Spec: ../spec.md

Deliver scene-imported textures bound by explicit shader role, preserving legacy
materials and instance inheritance. Material values must match compiled packing.

Acceptance criteria:
- Explicit exported texture parameter name wins over a surface suffix.
- Unmatched parameter names retain suffix and asset-name legacy fallbacks.
- Multiple layer textures and instance overrides remain independent.
- Scalar/vector padding and skipped padding members do not corrupt values.
- Existing material-import integration tests and targeted new regressions pass.

Verification: MaterialImportTests with actual DirectX compilation/reflection;
engine/solution build coordinated with root. Read repo standards and guides.

## Verification

- Explicit texture parameter roles now precede legacy suffix and asset-name binding.
- Reflection uses compiled top-level `StartOffset` and relative structure member
  offsets. Skipped padding retains its compiled space. Material creation rejects
  a buffer exceeding 128 bytes before copying any exposed members.
- Real MaterialImportTests red/green observed on NVIDIA GeForce RTX 4060:
  binding red at 17:37:11, green at 17:38:35; fixed-offset packing red at
  17:39:08, green at 17:39:30; safe hidden-padding overflow rejection red at
  17:40:28, green at 17:40:44 (2026-10-22).
- Final extended integration run passed at 17:41:24: three independent surface
  texture roles, an instance override with a conflicting suffix, inherited
  layers, legacy normal suffix and asset-name fallback, scalar/vector values at
  fixed HLSL offsets, skipped padding, nested member offsets, 144-byte rejection.
- Build: VS 2026 MSBuild `Tests/GameFramework/MaterialImportTests.vcxproj`, Debug
  x64, `/m:2 /nr:false /p:DebugInformationFormat=OldStyle
  /p:GenerateDebugInformation=false /p:LinkIncremental=false`. Sandbox compiler
  attempts stalled; cancelled and reran with approved escalation. Existing
  LNK4075 and missing FMod PDB LNK4099 warnings remain. `git diff --check` passed.
- Pending: integrated solution build and independent feature review by root.
  Rendering appearance is outside this ticket's integration-test evidence.
- Follow-up at 17:50:21: optional `MaterialImportTests.exe <prepared-scene.json>`
  passed against the staged runtime scene through `UnrealSceneImporter`. All
  eleven expected texture roles/assets survived in the master and three mesh
  material uses; noise vector values and scalar debug value, terrain/floater
  actors, camera FOV and converted position were checked. The actual prepared
  `MM_Landscape.hlsli` compiled through `GraphicsEngine::CreateMaterial`, with
  `MB_DEBUG` at byte 64 and `BlendNoise` at t14. The no-argument integration suite
  also passed after this change. `.scratch` is not needed for the normal test run.
