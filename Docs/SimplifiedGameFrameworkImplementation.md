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
- M2: pending — object, transform and lifetime contracts.
- M3: pending — owned scene data, registered readers and scene service.
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
