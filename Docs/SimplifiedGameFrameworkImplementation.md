# Simplified game framework implementation

## Starting point and validation

- Requested implementation branch: `codex/simplified-game-framework`.
- Base: `deferred-rendering-optimisations`, `8761675080371eb67abac8ed805c551360275cf5`, exactly the revision inspected by the plan. No newer base changes required reconciliation.
- Initial working tree contained only the untracked `Docs/SimplifiedGameFrameworkPlan.md`; it is preserved in the initial documentation commit.
- Full plan and ancestor/repository instruction locations were inspected before edits; no applicable `AGENTS.md` was present.
- Visual Studio 18 Community supplies v145. Baseline ModelViewer, GameFrameworkTests, EngineOptimisationsTests and GameFrameworkHostTests builds passed in Debug and Release x64 (8 configurations). Framework and optimisation CPU executables passed in both configurations. Host Release: valid, invalid/recovery, initial-invalid and begin-failure passed in threaded and sync modes (8). Host Debug: valid and begin-failure passed in both modes (4); invalid and initial-invalid abort with exit 3 at the existing unconditional content assertion in both modes (4 pre-existing failures). Logs: `Intermediate/SimplifiedFramework/baseline-*.log` (generated, untracked).
- MSBuild file tracking initially failed under sandbox with E_ACCESSDENIED; approved execution outside the sandbox built successfully. Existing Camera3D unused-parameter and CommonUtilities missing-PDB warnings remain. GPU host tests used the available NVIDIA RTX 4060; these are not visual comparison evidence.

## Milestone progress

- M1: complete — core Public include facade; private World/Actor dispatch and ownership collections; private input bookkeeping; GameContext Pimpl; engine built-in registration followed by game registration and engine freeze; world camera selection; explicit legacy scene integration bridge for ModelViewer/host tests only.
- M2: complete — safe Transform/LocalPose, immediate parenting/admission checks, consistent pending queries, duplicate display-name handling, private Actor construction, stricter refs, read-only dependency resolution, RAII cleanup.
- M3: complete — owned scene data, registered readers, authored fixups, candidate-local ready assets, scene service and ModelViewer C++ source.
- M4: pending — renderer boundary and ModelViewer migration.
- M5: blocked by external inputs — repository inspection found `ThirdParty/TGAFBXImporter`, which imports mesh/animation assets, but no actual Perforce scene importer, current headers/changelist, exporter implementation, or representative scene exports. The historical `ImporterHandoff.md` is not evidence of a verified contract.

## Decisions and review

Implementation preserves the existing facade and renderer. The testing agent owns test files; the independent reviewer is read-only. Each milestone requires executed regression checks and resolution of actionable review findings. No merge, push or pull request is authorized.

## Limitations and outstanding evidence

Baseline results above precede production edits. No visual compatibility or Perforce integration is claimed. Runtime/visual checks and exact integration blockers will be recorded as work proceeds.

M5 requires a pinned Perforce changelist and importer/asset owners; owned result and explicit error contracts; stable custom type names/payload preservation; actor/component ID scopes; units, axes, matrix and parent-local versus actor-relative conventions; asset mapping and material-slot ownership; light/camera conventions. Required fixtures include a valid level, a custom behavior, multiple same-type components, three-deep transformed hierarchy with expected matrices, malformed matrices, missing/unknown references/types, ordered material slots, and asset failures. No competing parser or guessed exporter adapter will be introduced.


## M1 decisions and independent review

- Registry types use stable agp.* engine names; ModelViewer registers only its four behaviors in the IGame setup hook.
- Closing is a separate flag from Ending, so game Shutdown retains borrowed world access while construction and scene requests reject. Completed component EndPlay follows game Shutdown.
- Review found direct bootstrap Prepare failure performed eager teardown before game Shutdown. Removed that eager teardown and its assertion; common cleanup now preserves borrows until Shutdown completes. The nonasserting bootstrap error change is pulled forward from M3 to establish/test M1 shutdown semantics. Legacy factory content assertions remain until M3.
- Temporary Public forwarding headers and Integration LegacySceneBridge have explicit removal milestone M4 and M3 respectively. ModelViewer and host regression fixtures are the only legacy scene bridge consumers.
- Transform math implementation moved out of headers without changing math behavior; safe facade semantics are M2.

### M1 validation

Independent testing agent executed ModelViewer, framework, optimisations, host and PublicGameplayConsumer builds in Debug/Release x64: all 10 passed. Framework and optimisation CPU suites passed in both configurations. Host checks passed 28/28: direct bootstrap, registration failure, Initialize failure, bootstrap validation failure, valid replacement and BeginPlay failure in both modes/configurations, plus legacy invalid/recovery and initial-invalid in Release. The four known Debug legacy-factory assertion cases were not repeated; M3 removes that policy. Public include traces contained no DirectX, D3D, Windows, RHI, GraphicsEngine or internal scheduler headers. Logs: Intermediate/SimplifiedFramework/m1-*.log. Independent review's bootstrap teardown finding is fixed and covered by bootstrap-invalid in Debug/Release, threaded/sync.

A preserved baseline Release executable was launched with computer-use and its lit scene captured under Intermediate/SimplifiedFramework/visual-baseline/lit.png (executable SHA256 250075F72FE82E86711403035892E8CBFEB7AB610EFEA9F649A3E222DD07A48C). Floor, opaque/alpha chests, checker and character rendered. Automated keyboard attempts did not establish R/F7/F6 behavior; hierarchy-named capture is only an attempted control capture, not hierarchy evidence. These captures are qualitative, with uncontrolled animation timing, not pixel equivalence or a full visual acceptance pass.

## M2 decisions and independent review

- Actor runtime identity remains its checked slot/generation token, separate from mutable display labels. Authored ID mapping is introduced with owned scene records in M3; the temporary recipe source still uses unique IDs as its labels.
- ResolveReferences uses a serialized-thread guard across worlds. Engine structural/property edits, scene requests and quit requests throw; a swallowed mutation exception still rejects validation. Custom C++ fields/external side effects remain the component author's read-only contract.
- Pending typed lookup required moving begun checks into existing mesh/light extraction now, ahead of M4 relocation; otherwise a component added during Update could render before BeginPlay.
- Invalid runtime addition batches now report errors and retain established objects in Debug too; this part of the nonasserting policy moves forward from M3 to support the M2 failure contract.
- Review found RequestQuit could bypass resolution restrictions and terminate a retained scene. It now uses the same guard; the independent resolve-quit host regression passes in both modes/configurations.
- The sample camera controls use the same input sample, speed, sensitivity, pitch clamp and movement logic through the safe facade. The old utility controller remains available for unrelated consumers but is no longer a gameplay header dependency.
- Camera degenerate-axis fallback under zero inherited scale is recorded for M4 camera/extraction validation. No new renderer passes were changed.

### Supporting logger shutdown correction

New warning/error regression paths exposed a Release process-exit hang absent from the initial baseline cases. The first fix closes an existing condition-variable lost-wakeup window by changing the stop predicate under the wait mutex and draining queued entries. That correction alone did not resolve the observed hang. Instrumentation then showed every CPU test completed while the log worker had emitted nothing; its first Timestamp call dynamically initialized static strings during CRT teardown. Replacing those format strings with constant literals removes late destructor registration from the worker. The Release CPU suite passes; ten bounded diagnostic-and-exit runs all returned zero and emitted every queued message. Independent review passed. This supporting change is required to execute the framework failure-path tests reliably and does not alter logging APIs.

### M2 validation

Independent testing passed all ten project/configuration builds (ModelViewer, framework, optimisations, host and isolated public consumer, Debug/Release x64). Framework and optimisation CPU suites passed in both configurations. The host matrix passed 36/36 cases across Debug/Release and threaded/synchronous modes, including rejected runtime additions and swallowed resolver quit attempts. After the logger correction, ModelViewer/framework/public/host builds were refreshed successfully in both configurations and CPU suites rerun. Tests cover pending lookup, duplicate names, multiple same-type components, all spawn phases, immediate hierarchy, invalid poses, read-only resolution across worlds, stale refs and exception cleanup. Public include traces remain isolated. Logs: Intermediate/SimplifiedFramework/m2-*.log. M4 will perform complete visual/control acceptance; none is inferred from these automated checks.

## M3 decisions and independent review

- Owned SceneData records use scene-local actor IDs and actor-local component IDs independently of display labels. Source metadata is propagated into aggregated typed diagnostics. Missing data differs from a valid empty scene; generic scenes do not require a camera, while ModelViewer explicitly requires one.
- Registry readers use a small owned typed map with strict consumed-field checking. All objects allocate before configuration; authored Ref destinations resolve after configuration and readers/fixups die before failed candidate objects. Direct C++ construction shares the same validation/start lifecycle.
- Initial review found custom factories received an Actor and could mutate a retained world. Factories now return unattached unique_ptr components under a read-only engine guard; engine attachment follows outside the guard. Readers may mutate only their target component through engine setters. Captured arbitrary C++ fields/external side effects remain author contracts.
- Camera validation now checks both input domains and the resulting projection matrix; finite extreme inputs that overflow no longer replace the existing projection. Review also caught bad_alloc being converted into recoverable source errors; allocation failure now follows fatal session cleanup.
- ApplicationSetup owns one ISceneSource. SceneService stores identifiers only, last request wins, and Load/Reload/status/results replace all candidate-world gameplay APIs. The old recipe and LegacySceneBridge files are deleted. Old-world EndPlay requests reject; new BeginPlay requests queue for the next boundary.
- Recoverable source/data failures report SceneLoadError in Debug and Release (E15 updated). Initial requested load failure reports, cleans up and returns 1. Direct bootstrap programming validation and postcommit callback exceptions still throw after cleanup. Press/mouse/timing transients clear before loading callbacks; elapsed GameTime is session-wide gameplay time and excludes loading.
- AssetBindings is a candidate-local view over ready shared resources, not a new loader/cache. ModelViewer retains its existing MeshLibrary/material backend in the source unit and creates a fresh alpha instance per load. The old view survives until old-world teardown finishes; snapshots continue retaining backend resources.
- Independent M3 builder/integration review accepted identity mapping, fixup lifetime, optional-camera policy, strict properties, source/resource ownership and commit ordering after these corrections. Automated results are recorded below. Renderer exposure/no-camera presentation are explicitly completed in M4.

### M3 validation

Independent testing passed all ten Debug/Release x64 project configurations, framework and optimisation CPU suites in both configurations, and 80/80 real-host cases (20 scenarios × both configurations × threaded/synchronous). Coverage includes strict typed/default/unknown properties, source diagnostics, IDs and forward/cyclic references, factories, invalid assets/material slots/cameras, reader/factory/resolver mutation and allocation failures, adapter failure/throw, initial nonzero failure, latest request/reload, old-ref retention/expiry, result callback ordering, BeginPlay requests and teardown request rejection. Result/Begin callbacks observe reset input/delta; synthetic Windows key presses were not injected. Fixed-input/accumulation semantics remain separately exercised by CPU regressions.

A no-device extraction fixture passes before M4 in Debug/Release: three eligible meshes with pending mesh/light excluded; known offset matrices and scaled bounds; opaque order {1,2,0}; skeletal joint translation 8; camera z=-100; light position (12,29,42); retained mesh/material ownership after world destruction and release on snapshot Clear. This records numerical extraction/resource behavior, not visual equivalence. Logs: Intermediate/SimplifiedFramework/m3-*.log. Final host commit-order refresh and ModelViewer source runtime smoke are recorded before the M3 commit.

Computer Use was stopped by a physical Escape during the attempted M3 ModelViewer runtime inspection. No further UI inputs were issued. M3 visual/control acceptance is unperformed; automated host/extraction tests do not substitute for it. Final reporting retains this limitation.

Final M3 refresh after commit-order hardening: ModelViewer and host Debug/Release builds passed; 16/16 targeted valid/invalid/new-Begin-request/completion-failure cases passed across both modes/configurations. Actual ModelViewer source runtime remains for automated M4 source smoke; no visual smoke was performed.
