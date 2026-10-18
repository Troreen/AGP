# 01: Safe EventBus subscription lifecycle

Status: resolved
Blocked by: none

## Parent and implementation gate

[Collision detection MVP spec](../../spec.md), criterion T1. Read [workflow](../../workflow.md): source implementation is authorized by the resume request; see workflow.md.

## What to build

Make the existing shared EventBus safe for gameplay listeners that subscribe/unsubscribe while handling a notification. Preserve the public AddEventListener/RemoveEventListener/Dispatch model and synchronous delivery. This prerequisite is directly testable without colliders.

## Acceptance criteria

- [x] EventBus::ListenerID is public and usable as a gameplay Component member.
- [x] A callback can remove itself safely.
- [x] Removing another listener before its turn prevents its call in the current dispatch.
- [x] Added listeners do not join the current dispatch; subsequent and nested dispatches use their own snapshot.
- [x] Removing the last listener cannot invalidate the in-progress dispatch.
- [x] Nested dispatch, missing event types, and removal of unknown IDs remain safe.
- [x] Preserve exception propagation and existing callers; no new event queue or replacement bus.

## Verification

Use existing framework test conventions and the public EventBus API. Run the affected test executable and relevant existing regressions. Include mutable callback-state persistence across repeated dispatches to avoid fixing iterator invalidation by inadvertently copying away callback state. Record commands/results when implemented.

## Comments

Implemented in EventBus.hpp with shared listener entries retained by each dispatch snapshot. Removal marks an entry inactive before removing it from the live list. Snapshots preserve callable identity and mutable callback state; deleting the last live event list cannot destroy an executing callback. Public AddEventListener/RemoveEventListener/Dispatch signatures are preserved.

Root implementation review found behavior consistent with the spec; final independent spec and standards reviews are part of integrated feature verification.

## Verification evidence

- TDD red: initial public token test failed with MSVC C2248 (ListenerID private). Exposing the existing alias made that slice pass.
- TDD red: public removal-before-turn test failed with `bad function call` against the previous vector dispatch. Entry snapshots made the slice pass.
- `MSBuild.exe Tests/GameFramework/EventBusTests.vcxproj /m /nologo /v:minimal /p:Configuration=Debug /p:Platform=x64`, then `Bin/Tests/Debug/EventBusTests.exe`: pass.
- Same focused build/run in Release: pass. Both use v145, C++20, warning level 4, and warnings as errors.
- Six focused tests cover public IDs, another-listener removal, self-removal of the last listener followed by nested dispatch, addition deferred until the next dispatch, mutable callback state, unknown IDs/missing types, and exception propagation.
- `git diff --check` for owned source/test paths: pass.
- Fresh `GameFrameworkTests.vcxproj` Debug build attempted: blocked by pre-existing GraphicsEngine.cpp:26 C1083 for missing `Lines_PS.h`. Existing Lines shader files are absent from GraphicsEngine.vcxproj FxCompile membership. No unrelated graphics source/project edits were made. Ticket remains in-progress until the fresh existing-regression gate passes.
- Resume retry after root registered existing Lines shaders: GraphicsEngine and GameFramework libraries build successfully. Legacy GameFrameworkTests compilation is still unavailable because it uses removed ServiceLocator setters, treats reference-returning GetInputMapper as a pointer, and references absent TGAFBXImporter/source/*.cpp sources. These failures predate EventBus changes. No legacy tests or application settings were modified. Current regression substitution is being coordinated by the feature owner.
- Feature owner approved current `CustomComponentTests` as the relevant framework lifecycle regression substitute; repairing the legacy suite's unrelated service APIs and importer setup is outside this ticket. Fresh CustomComponentTests initially exposed existing missing LineDrawer and BehaviourTree source membership. Root registered existing LineDrawer sources in GraphicsEngine; this ticket added the existing BehaviourTree/**/*.cpp pattern to CustomComponentTests.vcxproj, matching GameApplicationTests. No runtime behavior changed for these metadata repairs.
- `MSBuild.exe Tests/GameFramework/CustomComponentTests.vcxproj /m /nologo /v:minimal /p:Configuration=Debug /p:Platform=x64`, then `Bin/Tests/Debug/CustomComponentTests.exe`: pass (`PASS: custom property import and SpinComponent creation`). Existing unused aDeltaTime warnings remain in application code. Fresh framework libraries and current custom Component regression now pass; ticket 01 is resolved. The legacy GameFrameworkTests limitation remains recorded for integrated verification.
