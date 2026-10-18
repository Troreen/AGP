# Workflow
Stage: verification; awaiting external Unreal authoring, remaining human visual checks and Release dependency repair
Original base: 545e62a5e7d4bb529bc5e0bcfbff8b8e49aa5036
Integration branch: shader-testing-scene
Spec: spec.md
Initial Git scope: unstaged Bin/Settings/ApplicationSettings.json only; no staged or untracked files. Preserve this user change.

## Ownership
Root: Game.cpp manual actors, application scene wiring, planning records, builds/integration.
Renderer agent: GraphicsEngine source/projects and matching engine shader copies.
Asset agent: offline converter and generated DDS.
Unreal source scene/exporter: external, location requested from user; no substitute code-spawned reference meshes.

## Decisions
See spec.md. User approved five-task plan including global manual-character removal. Planning concluded; implementation explicitly authorized. No staging/commits.

## Next action
Implement renderer/environment, convert HDRI, disable manual actors, locate Unreal source, build and independently review. Keep visual acceptance and unavailable Unreal authoring pending.

## Implementation and review progress
- Manual Player, Enemy and A_Debug setup blocks commented out globally, preserving code.
- Offline converter and HDR-capable DDS cube completed (512-square faces, six faces, ten ordinary mips). Original source preserved; see environment-conversion.md.
- Renderer environment/skybox support and shader-scene-only application wiring implemented.
- Independent spec review: 0 confirmed defects. Independent standards review: 0 findings. See review.md.
- User confirmed Unreal project is unavailable on this device. Task 04 remains ready-for-human; exported geometry and directional intensity 4 are deliberately unchanged. See unreal-and-visual-checks.md.
- Debug solution build running; Release and focused runtime/input checks next.
- User preference for built-in follow-up questions saved to the permitted memory extension notes folder.

## Runtime discovery and user scope update
The first logged launch loaded Content/EAP.json, which takes precedence over InitialScene and was identical to the named shader scene. It therefore bypassed the original named-scene-only environment condition and matched the reported black background. User chose to keep EAP-first startup, enable the fixed environment for regenerated EAP exports too, and explicitly delete the current generated Content/EAP.json. Deleted only that exact file. Application environment matching now compares normalized resolved scene paths for both named shader scene and EAP. Other named scenes retain default environment.

Computer-use capture failed with FrameArrived timed out, then failed to activate captured window on one refreshed retry. No visual correctness is claimed. Initial logged startup smoke reached EAP scene construction; final named-scene smoke and build outcomes are recorded below.

## Verification evidence
- Initial Debug Game.sln solution build passed (build-debug.log). Latest application rebuild after EAP scope correction passed (build-debug-app-final.log); BuildProjectReferences=false reused the successfully built current Debug libraries. A redundant dependency rebuild was interrupted to avoid rebuilding identical libraries solely for differing SolutionDir spelling.
- Release feature source and libraries compiled, but Game linking failed C1007 unrecognized flag -permissive- in p2 / LNK1257 (build-release.log). Read-only dependency inspection found SoundEngine-FMod.lib embeds MSVC 14.51.36231 LTCG members; installed active compiler/linker is 14.50.35717/14.50.35728. No vendor/toolchain changes made. Final Release executable remains unavailable.
- Initial focused control-test build exposed a stale compile entry for deleted EnemyComponent.cpp (deletion verified in commit 945500d). Removed that single project reference; final focused build/run evidence follows.
- Final named-scene startup log: Game.log at 22:18:41 loads Lvl_Shader_Testing_Tarik_Level with two exported actors; material compilation and FBX assignment complete by 22:18:45. Process remained responsive with no environment-load failure logged. This proves startup/scene-loading smoke only, not visual acceptance.
- Existing snow-material parameter-binding warnings remain unchanged; not introduced or repaired in this environment work.
- Computer-use visual capture unavailable after one refreshed retry: FrameArrived timed out, then failed to activate captured window. F4, visual orientation/depth/debug-view behavior, other-scene comparison and final user lighting acceptance remain pending.
- Exported directional intensity remains 4 and reference geometry is not added because user confirmed Unreal source is unavailable on this device. Task 04 remains ready-for-human.

## Human runtime evidence
User confirmed: the snowy hillside background skybox is now visible. This establishes skybox visibility after the EAP path correction. It does not establish F4 cleanup, debug-view exclusion, reflection alignment, translation behavior, or final lighting balance. Follow-up reload-check question was unanswered; retain that check as pending.

## Final available-check results
- Focused test project rebuilt successfully after the single stale source-entry repair (build-controls-tests-final.log).
- GameApplicationTests.exe camera-controls: PASS (debug-camera controls and automatic frame-time chest spin).
- GameApplicationTests.exe render-pass-controls: PASS (F5/F6 render-pass controls).
- Final independent spec and standards reassessment: 0 confirmed findings each.
- git diff --check passed. Nothing staged or committed. Pre-existing settings diff remains unchanged (22 additions / 8 removals). Automated smoke process stopped after recording startup; user subsequently confirmed the skybox is visible in their run.

## Continuation
Next action: perform the checklist in unreal-and-visual-checks.md when Unreal source is available; obtain remaining visual acceptance. Resolve the prebuilt FMOD/toolchain mismatch separately before Release linking can pass. Current feature remains in-progress; no claim that every planned task is complete.

## Simplification after review (2026-10-19)
User requested removing speculative safeguards. Scene selection now compares the constructed paths directly; removed lexically_normal calls and their temporary path variable. Removed the extra Texture::IsCube API/type check for the fixed, generated cubemap, returning Texture source/header to their original contents. Kept the agreed scene-load failure behavior and default environment restoration. Debug build evidence: build-debug-simplified.log (outcome to follow).
Debug solution build passed after simplification. git diff --check passed; changes remain unstaged and uncommitted.

## Commit authorization (2026-10-19)
User explicitly requested a commit. Include the scoped engine/application changes, generated runtime cubemap, EAP deletion, test-project repair, and durable feature records/converter. Exclude the pre-existing Bin/Settings/ApplicationSettings.json change and generated build/runtime logs, process IDs and preview image. Unreal authoring, remaining visual checks and Release linking are still pending as recorded above.

## Bloom and lighting comparison (2026-10-19)
User reported excessive widespread glow that existed before the HDRI. Temporarily disabled only bloom via SetActive(false), rebuilt and launched Debug: user reported improvement but remaining excessive brightness. User authorized direct local exported-JSON tuning because Unreal source is unavailable. Directional intensity 1 was too dark; intensity 2 with bloom off was accepted after a confirmed running comparison. Re-enabled the original bloom, rebuilt and launched Debug at intensity 2: user confirmed it looks good. Removed the temporary bloom-disable line; no bloom shader or implementation changes remain. Final change: exported directional intensity 4 -> 2. Existing bloom stays enabled with threshold 1.3 and exponent 1.5. Sync intensity 2 to Unreal source later. This new adjustment remains uncommitted.

## Machine handoff (2026-10-19)
User requested committing and pushing all current changes to continue on another machine. Current branch: shader-testing-scene. Current accepted rendering: hillside environment/skybox, directional intensity 2, original bloom enabled. Debug build and camera/render-pass controls passed; user accepted skybox visibility and light/bloom balance. Remaining work: Unreal floor/reference spheres and syncing intensity 2 back to the source scene; outstanding visual checks; Release link gate caused by SoundEngine-FMod.lib built with MSVC 14.51 versus installed 14.50. Recheck the destination machine's toolset before retrying Release. Preserve EAP-first startup; generated EAP.json was deliberately deleted and future EAP exports use the hillside. This handoff includes current application settings and all Git-visible verification artifacts. Nothing is merged to another branch.
