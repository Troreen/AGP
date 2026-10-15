# Scripted events workflow

- Request: implement agreed component-based scripted events using agp-feature. Unreal unavailable; authoring setup tomorrow.
- Initial base: 81993dbc001aebbe4bd3f21ae9591ad8b47d1f53 on main.
- Integration branch: scripted-events (attached checkout; independent agents own disjoint files).
- Initial staged/unstaged: none. Pre-existing untracked: Docs/ScriptedEvents.png; preserve and exclude from commits.
- Stage: implementation and independent review verified; awaiting manual runtime/editor checks.
- Spec: spec.md. Tickets: issues/01-runner.md through issues/04-handoff.md.
- Public test seams: World/Actor lifecycle, sequence Play/Cancel/status and actions; existing custom-component importer/builder. Runtime visual/audio and Unreal round trip remain distinct checks.
- Owners: root runner/actions/integration; playback_support engine playback; player_control player support then behavior tests; unreal_handoff guide.
- Verification: see verification.md. Game, ScriptedEventsTests, CustomComponentTests, GameApplicationTests pass; startup and normal window close pass. Legacy GameFrameworkTests and strict public-header check have recorded existing failures.
- Review: both independent axes reassessed fixes; no remaining actionable findings.
- Next: perform the F2 visual/audio/reload checks in verification.md, then tomorrow's Unreal setup/export round trip. Reuse the existing uncommitted implementation; do not recreate or automatically commit it.

- Git correction: user requested removal of automatic commit 01cb698. Reset branch to original base 81993dbc001aebbe4bd3f21ae9591ad8b47d1f53 with all feature files retained unstaged; renamed branch to scripted-events. Future commits/staging require an explicit user request.

- File-review follow-up: registration moved to Components/RegisterCustomComponents.cpp; Animator assumes a stable mesh component at runtime and no longer enumerates components during updates. GetComponents docs clarify all-match lookup. Staging must remain exactly as the user selected it.

- Readability follow-up: moved the six scripted-event sources to Components/ScriptedEvents, updated source includes and project/filter/test paths, expanded compact blocks/functions and validation branches using repo formatting. No staging or commits performed.

- Latest verification: relocated/formatted Game build and all three focused test suites pass. No staging or commits performed; manual visual/audio and Unreal export checks remain as previously recorded.

- User review: Animator preflight now checks assigned mesh via HasMesh() and registered animation only. User approved one registered audio playback per SoundID; removed scripted instance pool, reuse flags, Release API, and repeated playback-list searches. Audio actions now start/check/stop by SoundID. Shared IDs cannot provide independent overlapping scripted playback.

- Verification of simplified audio/mesh checks: integrated Game and all three focused test suites pass; see verification.md. User staging preserved; no commits or staging performed.

- Agreed animation handoff: removed the saved playback snapshot. Ending scripted playback restarts the tree's current clip from frame zero without repeating state entry/exit events or changing variables. Actors without a tree hold their scripted pose until another animation is requested. CPU-only public API tests cover both paths, variable preservation, resumed transitions, and a same-clip override. User staging must remain unchanged.
