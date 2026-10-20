# World-space snow workflow

- Request: add snow shading to the existing shader-test master material using world vertex positions.
- User decision: upward-facing surface coverage, rather than a world-height cutoff.
- Original base: d627d712a1024b4cfb7f1b623692e51ef4714ae9 (main).
- Initial staged, unstaged, untracked scope: empty (clean checkout).
- Integration branch: world-space-snow; attached checkout; no additional worktrees.
- Stage: verification. Ticket 01 implemented by snow_implementation; coordinated by root.
- Spec: [spec.md](spec.md). Ticket: [01-snow-material.md](issues/01-snow-material.md).
- Decisions: actual exported material name is MM_ShaderTest; add its shader at the engine shader root, which the existing build copies beside Game.exe. World up is +Y. Use position derivatives for geometric surface direction and orient against the provided world normal. Keep the shared material pipelines unchanged.
- Verification seam: compiled material programs plus the existing shader testing scene. No synthetic CPU copy of shader math.
- Constraints: no staging, commits, publication, or unrelated edits. Required visual evidence keeps ticket in-progress if unavailable.
- Evidence: Surface_VS, GBuffer_PS, Lit_PS, Unlit_PS all compiled with fxc shader model 5.0 using the actual new material include. Outputs under Intermediate/WorldSpaceSnowVerification-20261016. An initial probe reused a stale ignored verification folder and failed due to old includes; the isolated probe passed.
- Build: initial standard build exposed existing missing Lines_PS/VS generated headers and LineDrawer.cpp membership. Generated ignored headers with fxc and used an ignored LocalBuildPrerequisites.targets import to include existing LineDrawer.cpp; Debug x64 Game.sln then passed. User subsequently updated tracked project membership concurrently; preserve those changes.
- Runtime: launched rebuilt Game.exe and inspected screenshot via computer-use. Sphere showed a white cap, softly graded boundary, gray lower surfaces, and lighting. Rotation/translation, alternate camera, and ordinary-material comparison checks remain pending; interrupted by the user's audio request.
- Independent reviews: review_spec and review_standards both reported zero implementation findings. Visual acceptance remains partially pending.
- Concurrent user changes observed after interruption: Bin/Settings/ApplicationSettings.json, exported shader test scene JSON, GraphicsEngine.vcxproj and .filters. These are outside agent edits and must be preserved.
- Added user request: disable per-frame FMOD listener console diagnostics. Disassembly confirms hardcoded std::cout prints in prebuilt SoundEngine-FMod-d.lib, without available source/logging switch. AudioManager.cpp now scopes cout suppression to SetListenerPosition and restores stream state via RAII. Listener updates and position scaling are retained. Standards reviewer found no defects; temporary suppression can also affect simultaneous cout writers.
- Next action: finish audio build/runtime check; resume remaining snow transform/camera/comparison checks when the scene is available for controlled testing.
- Retrospective suggestion (not applied): regenerate/check project membership when existing source or shader files are added; missing line-drawer membership blocked a clean build here.

## Unreal graph follow-up

- User provided Docs/ShaderUnreal.png (initial chat attachment error, but file exists and was inspected). New requested target is MM_Standard_WithSnow.
- Current spec: [unreal-reference-spec.md](unreal-reference-spec.md); ticket [02-unreal-reference.md](issues/02-unreal-reference.md). This replaces the earlier derivative-based design for delivery; retain MM_ShaderTest as the earlier prototype.
- Implemented the reference's mapped PixelNormalWS -> multiply 4.5 -> add -2.5 -> saturate -> lerp(0.2, 0.9), adapting Z-up to +Y. Added VertexNormal to the internal material-hook input, initialized by GBuffer, Lit, and Unlit, for Flatness control. No new GPU constant buffer layout or vertex interpolator was added.
- Verification: all four shader programs compile for new material, previous MM_ShaderTest, and _DefaultMaterial (12 fxc invocations, all passed). Standard Debug x64 Game.sln passed. Output: Intermediate/UnrealSnowVerification-20261016.
- Independent spec and standards reviews found no confirmed defects. Existing unlit pipeline does not sample normal maps; use the opaque/deferred test material. Lit debug views read pre-hook samples; deferred Albedo is the correct color check.
- Visual verification interrupted by user request to debug the active material; no temporary edits to EAP.json were made.

## Active-material diagnostics

- User requests confirmation of which material/shader is on the shader-test object.
- Confirmed file mismatch: Content/EAP.json assigns MM_ShaderTest to SM_Debug_ShaderMesh_Sphere; the exported shader-testing scene assigns MM_Standard_WithSnow to the same actor. GameApplication::InitializeGameSession prefers existing EAP.json over settings.initialScene.
- Added scene-load Verbose diagnostics to SceneWorldBuilder::ApplyMesh showing actor, component, mesh name, slot, and actually assigned material name after SetMaterial succeeds. Added GraphicsEngine::CreateMaterial success diagnostic showing compiled material name and the actual material shader include path (including fallback paths). No per-frame output.
- Standard Debug x64 Game.sln build passed. Standards review found no issues in the diagnostic hunks.
- Runtime confirmed at 19:02:25 via the live game console: scene='EAP', material compiled='MM_ShaderTest', shader='Bin/Debug/EngineShaders/MM_ShaderTest.hlsli'; actor='SM_Debug_ShaderMesh_Sphere', component='StaticMeshComponent0', slot=0, assigned material='MM_ShaderTest_Inst'. The console also remained free of per-frame FMOD listener diagnostics during this run.
- Left the rebuilt game running. No scene files were changed to switch selection. Next action for snow validation: select/export the intended MM_Standard_WithSnow scene/material, then perform the pending visual comparison.
