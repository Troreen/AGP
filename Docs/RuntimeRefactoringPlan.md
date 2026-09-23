# Runtime readability refactoring plan

> Historical snapshot: input architecture descriptions below are superseded by [Input restoration](../InputRestorationPlan.md).

Status: final, revised after three independent plan audits covering readability, simplicity, behavior/architecture and scope. Planning only; no implementation changes.

Baseline inspected: `4c19440adc9db4c8f46439787ca69f5ca17c932f` (2026-09-23). Line references describe that baseline and will move during implementation. In code references, `Runtime/`, `Scenes/`, `World/` and `Rendering/` are relative to `Source/Engine/GameFramework`; `GameFramework/` and `GraphicsEngine/` are relative to `Source/Engine`. Bare Runtime filenames refer to `Source/Engine/GameFramework/Runtime`; `Tests/`, `Docs/`, `Source/` and command paths are repository-relative. The working tree was clean before this document was added.

## 1. Objective, scope, and execution rules

Make Runtime easier to read locally while preserving its public API, architecture, behavior, and lifecycle ordering. Change only existing files under `Source/Engine/GameFramework/Runtime`. This plan lives there too. Inspect other directories as needed, but do not edit their code, tests, project files, documentation, or build settings. Generated build outputs are not source changes.

Do not introduce a new runtime framework, service container, scheduler, event bus, scene manager, or platform interface. Do not add translation units: `GameFramework.vcxproj` explicitly lists sources, and adding one would require an out-of-scope project edit. Keep helpers in existing implementation files/private declarations. No public signature changes or caller migrations are needed.

The main implementing agent owns the complete lifecycle and integrates one step at a time. Use subagents extensively for bounded investigation and independent reviews, with explicit file ownership if delegating edits. In particular, do not give different writers concurrent ownership of `GameApplication.cpp`. Reviewers should challenge behavior preservation, not merely approve formatting.

## 2. Current architecture: observed behavior

### Ownership and dependency map

| Object | Responsibility and lifetime | Evidence |
| --- | --- | --- |
| `GameApplication` | Public synchronous entry point. Each `Run` constructs an automatic temporary `Impl` for that invocation; it is not a persistent heap PImpl member. | `Runtime/GameApplication.h:25–46`, `.cpp:362–365` |
| `GameApplication::Impl` | Borrows `IGame&`; copies `Config`; owns the moved `SceneSource` callable, context, component registry, HWND, device adapters, host input subscriptions, graphics command list/snapshot, diagnostics widget/font/timer, mouse anchor, startup flag, and debug camera service. Captured references in the source remain caller-owned. | `Runtime/GameApplication.cpp:51–92` |
| `GameContext` | Session state exposed to gameplay: owns input and one world, stores current/pending scene enum, content root/client size, quit flag and scene-request gate. Input is declared before the world, so it outlives the world's destruction. The world borrows input. | `Runtime/GameContext.h:9–61`, `.cpp:3–5` |
| `IGame` | Application-thread callbacks supplied by the caller; application updates components through `World`, not through the game implementation. | `Runtime/IGame.h:7–35` |
| `InputSystem` / `InputSubscription` | Bind device samples to named action states; dispatch callbacks; RAII tokens weakly reference listener storage. Context owns the input system; host/game/components own their subscriptions. | `Runtime/InputSystem.h:11–111`, `.cpp:13–218` |
| External systems | Graphics and assets are singleton access points; audio has singleton startup/shutdown; `ServiceLocator` borrows services. `ComponentRegistry` builds a candidate world; `WorldRenderer` translates the live world into a snapshot. | `GameFramework/ServiceLocator.cpp`, `AudioManager.h`, `Scenes/ComponentRegistry.cpp`, `Rendering/WorldRenderer.cpp` |

Keep the nested `Impl`: it hides Windows/graphics implementation types without burdening the public header. Keep `GameApplication::GetFontResource`: it is a narrow access bridge justified by `FontHandle` friending `GameApplication` in `Scenes/AssetRefs.h:60–71`, not an arbitrary forwarding layer.

The `Impl` destructor body destroys the HWND before its members are destroyed in reverse declaration order. Do not reorder members to group them visually. `World::~World` calls `Clear` (`World/World.cpp:13–16`), but this does not substitute for service cleanup. Runtime owns no worker loop: rendering may use workers internally, and `GraphicsEngine::RenderSnapshot` joins its borrowed-snapshot work before returning (`GraphicsEngine/GraphicsEngine.cpp:657–684`).

### Startup, in exact order

`GameApplication.cpp:94–229` currently performs:

1. Register window class, accepting `ERROR_CLASS_ALREADY_EXISTS`; create the window with current styles/dimensions.
2. Canonicalize content root. Acquire graphics singleton, initialize graphics with `Shaders`, then create the command list using short-circuit evaluation. Store graphics client size.
3. Enter the main `try` **only now**. Initialize/check assets; optionally resolve diagnostics font and create/configure its widget. Missing font logs an error and continues.
4. Initialize audio; provide input, audio, assets to `ServiceLocator` in that order.
5. Configure window input and disable its automatic mouse capture; install default bindings; subscribe host tonemapping and debug-camera controls, then conditional render diagnostics controls.
6. Call `game.Initialize(context)`. If a scene is pending, load it synchronously. Otherwise call the existing world's `BeginPlay` directly: this path does **not** call `ConfigureWorld` or install a fallback camera.
7. Mark initial world startup complete. Optionally show/foreground the window; optionally set diagnostics title. Prime a local timer, then enter the loop.

### Main loop and rendering handoff

`GameApplication.cpp:230–285`:

1. Check quit at loop entry. Drain **all** queued messages. For `WM_QUIT`, request quit, but still pass that message to input event handling, translation, and dispatch. Break after draining if quit was requested.
2. If pending, attempt scene replacement. Then update the timer, even if replacement failed recoverably, so load time is excluded from movement delta.
3. Query client size. If either dimension is zero, continue immediately: no ordinary timer/input/game/world/audio/render work. On size change, reset command list, resize or throw, then update context size only on success.
4. Update timer; update platform input state; read elapsed time and convert non-finite values to zero or clamp finite values to `[0, 0.25]` seconds.
5. Advance notification timer, capture a device frame, and dispatch input actions.
6. Call game update, world update, audio update, in that order with the same delta.
7. Build snapshot (`WorldRenderer::Build` clears it and synchronizes camera state); append visible notification widget with its current opacity.
8. Reset command list; render snapshot into it; execute and present **only** if `FinishCommandList` succeeds. A false return is not currently an exception.

A quit or scene request from input/game/world callbacks does not stop the current frame. Quit is honored at the next loop condition; scene changes at the next boundary. Do not add mid-frame early exits. Zero-size windows currently busy-poll; introducing sleep/wait behavior is a separate change.

### Input sampling and action semantics

`CaptureInputFrame` (`GameApplication.cpp:372–425`) focus-gates keyboard state. Enabled, focused right-button mouse look measures displacement from client center only after an anchor exists, then warps the cursor back and marks the anchor. First activation yields zero delta. Disabled/unfocused/button-up clears the anchor; `GetClientRect` failure inside the active path leaves its previous value unchanged. XInput is polled even while unfocused; `InputSystem` later suppresses action evaluation for unfocused frames.

`InputSystem::Update` (`InputSystem.cpp:136–212`) evaluates focused bindings in order: keys, mouse, gamepad buttons, sticks, triggers. For an action shared across bindings, the last active/nonzero assignment wins; inactive/zero bindings do not erase an earlier value. No axis accumulation or normalization occurs here. Key suppression requires the same key, a strictly larger required-modifier list, matching required/forbidden state, and inclusion of every base required modifier. It is not just a comparison of modifier counts.

The system registers known action names, sorts all tracked action names lexically, then dispatches Started/Ongoing/Ended. Ended uses the previous value. State is updated **after** callback dispatch, so a thrown callback leaves that action's stored state unchanged and interrupts the remaining update. Focus loss ends active actions. `Reset` silently clears action state; `ClearBindings` clears bindings then resets, without removing listeners or synthesizing Ended events.

`Dispatch` (`InputSystem.cpp:95–134`) snapshots listener count and visits listeners in registration order. Reset during dispatch marks removal pending; a later matching listener still receives this dispatch. Finalization applies removals on both normal and exceptional exit. New listeners are beyond that dispatch's initial count. Do not infer general reentrancy or safe callback-time container mutation guarantees from this mechanism; retain the storage/iteration model and treat those concerns separately.

### Scene requests and replacement

`GameContext::LoadScene` (`GameContext.cpp:7–15`) rejects a closed request gate or `SceneType::None`; otherwise overwrites one optional slot. `true` means accepted, not loaded. `ReloadScene` queues the stored current enum. Requests from Initialize are applied before the first loop iteration.

`LoadPendingScene` (`GameApplication.cpp:317–360`) has three existing conceptual regions worth making visually explicit, not splitting into a new scene subsystem:

1. Compute display name from pending scene **before** the construction `try`. Build a candidate through source, registry, and `game.ConfigureWorld` while the old world remains alive and scene requests remain accepted.
2. Rethrow `bad_alloc` directly. For other `std::exception`s from construction/configuration, log and call `OnSceneLoadFailed`; rethrow on initial startup, otherwise return with old world and pending slot retained. Non-standard exceptions and exceptions from the failure callback escape. Failure is not a complete transaction rollback: callbacks/assets can have side effects.
3. Outside the construction catch: close scene requests; clear old world/EndPlay; replace world; assign current enum from the **then-current pending slot**; silently reset input action state; clear mouse anchor; reset debug camera; reopen scene requests; install fallback camera if absent; BeginPlay; `OnSceneLoaded` with the original display name; finally clear pending slot.

Do not snapshot or consume the request at entry. Source/configuration callbacks can capture context and overwrite pending; current behavior reads it again during commit. Requests made by BeginPlay or OnSceneLoaded are erased by the final reset. A recoverable failure leaves a request to retry on later boundaries unless callbacks overwrite it. Exceptions from replacement/BeginPlay/OnSceneLoaded are fatal outer-cleanup failures, not recoverable construction failures.

### Shutdown and exception boundaries

| Exit | Current behavior | Constraint |
| --- | --- | --- |
| Failure before main `try` | No game Shutdown or explicit service teardown; `Impl` unwinds and destroys its window/members. | Do not move window/content/graphics initialization under catch. |
| Failure inside main `try` | Close scene requests; call game Shutdown even if game Initialize was never reached; catch/log an exception from Shutdown; clear world; clear locator, shut down audio, clear assets; rethrow. | Only Shutdown has a nested catch. A later cleanup failure can interrupt cleanup or replace the original exception. Do not promise unconditional cleanup. |
| Normal loop exit | Close requests; game Shutdown; world Clear; locator Clear; audio Shutdown; assets Clear; return 0. | This tail is outside the catch. A throwing Shutdown skips explicit world/service cleanup; member destruction still occurs. |

Evidence: `GameApplication.cpp:288–315`. Preserve subscription/resource destruction timing and window destruction timing. Do not add graphics shutdown or early widget/font/snapshot resets.

### Comments/documentation versus implementation

- `GameContext.h:26` says next-frame-boundary application. Actual behavior also includes immediate post-Initialize loading, retry retention, and final reset discarding callback requests. Clarify comments in Runtime.
- `IGame.h:31` says Shutdown runs when Initialize partially fails; actual protected region starts before Initialize and excludes early platform/graphics failures. Document the precise boundary without promising complete cleanup on every failure.
- `GameApplication.cpp:50` says the gameplay path is the loop in Run. After extraction, point readers to `RunMainLoop` and explain the per-invocation owner.
- `GameFramework/README.md` summarizes game -> world -> renderer, omitting input, audio and diagnostics; that is a summary, not authority for reordering.
- `Docs/GameFrameworkMVP.md` records historical test success. Current runtime test source still uses a former string scene API, whereas production uses `SceneType`. Historical logs/binaries do not establish present validation. No edits to those external docs/tests belong in this task.

## 3. Readability findings and principles

| Location | Concrete difficulty | Direction |
| --- | --- | --- |
| `GameApplication.cpp:94–307`, `Impl::Run` | Window fields, service setup, callback construction, lifecycle policy, frame mechanics and cleanup are interleaved. Catch boundaries matter but are hard to see. | Extract existing phases while keeping error policy in Run and frame sequence in one loop. |
| `.cpp:163–208`, host subscriptions | Meaningful callbacks are embedded in startup; several operations share a line. | One host-input setup phase, readable inline callbacks; no callback router. |
| `.cpp:254–285`, size/timing/render | Size skip and command-list preconditions are buried beside gameplay updates. | Name render-size preparation and snapshot submission; keep timer and update order visible. |
| `.cpp:317–359`, scene load | `world`, `context`, `name`, `myStarted` under-explain candidate ownership, phases and failure policy. | Better names, spacing, narrow comments; retain one auditable scene function. |
| `.cpp:383–410`, mouse look | A method called CaptureInputFrame also warps the OS cursor and changes an anchor. | Name that effect explicitly in a dedicated substantial mouse-look helper. |
| `InputSystem.cpp:148–162` | A nested multi-algorithm expression encodes a nontrivial key precedence rule. | A few private predicates with explicit control flow; keep device loops simple and local. |
| `InputSystem.cpp:13,27–31,195–210,228–267` | Compressed state changes, phase dispatch and tuple-driven shortcut installation hide ordering. | Expand statements, use descriptive locals, make light shortcuts explicit. |
| `GameContext.h:54`, `IGame.h:13–32` | Enum named like a string; callback parameter roles and lifecycle preconditions mostly absent. | Private rename and concise factual contracts, without expanding public APIs. |

Use a helper only when it names a meaningful phase or rule and makes its dependencies/effects clearer. Keep straightforward getters, binding append methods, the service-clear sequence, diagnostic log calls, and small callback bodies straightforward. Line count is not a success criterion. Keep side effects in execution order; name intermediate values when useful; do not generalize one-off logic or add interfaces to obtain shorter methods.

## 4. Proposed target structure

Keep all existing classes and file boundaries. No new production files. The following private `Impl` methods form the target; declarations stay inside the nested class in `GameApplication.cpp`:

| Method | Sole conceptual responsibility |
| --- | --- |
| `GraphicsEngine& InitializeWindowAndGraphics()` | Exact current pre-try window/content/graphics setup, returning the already-acquired singleton reference. Acquiring it earlier via a caller argument would change initialization order. |
| `void InitializeServices()` | Asset initialization/check, optional diagnostics widget setup, audio startup, locator registration, in current order. Retain widget construction inline as a labeled block. |
| `void InitializeInputAndHostControls()` | Configure device adapter, install defaults, then install current host subscriptions and conditional diagnostics callbacks. |
| `void StartGameSession()` | Initialize callback, first world startup, startup flag, window visibility/foreground, initial diagnostics title. |
| `void RunMainLoop(GraphicsEngine& graphics)` | Own local timer, boundary decisions, input and game/world/audio order, render handoff. |
| `void PumpWindowMessages()` | Drain messages with current quit/input/dispatch semantics. |
| `bool PrepareRenderTargetSize(GraphicsEngine& graphics)` | False means zero-size frame must be skipped. True permits the frame, including when no resize was needed. Otherwise resize if needed, preserving command reset and successful size update. Throws on resize failure. |
| `void RenderFrame(GraphicsEngine& graphics)` | Build fresh snapshot, add diagnostics text, record/finish/execute/present under the existing finish-success condition. |
| `void CaptureMouseLookDeltaAndRecenterCursor(InputDeviceFrame& frame)` | Consume `frame.Focused` and `frame.KeysDown` populated immediately beforehand; apply config/button decisions, mouse delta, cursor warp and anchor mutation. Never resample focus or buttons. |

Existing `LoadPendingScene`, `ShutdownServices`, `CaptureInputFrame`, title/notification/log methods remain. Avoid further methods just for ShowWindow, timer priming, a game/world update call, or a single event forwarding call. Keep singleton dependencies visible within the service/input/diagnostic methods and pass the existing graphics reference to loop/render helpers. Do not add a dependency bundle or store a redundant graphics member.

Intended `Run` shape (illustrative, not implementation):

```cpp
GraphicsEngine& graphics = InitializeWindowAndGraphics(); // outside try
try
{
    InitializeServices();
    InitializeInputAndHostControls();
    StartGameSession();
    RunMainLoop(graphics);
}
catch (...)
{
    // Keep the existing explicit exceptional cleanup sequence and nested catch.
}
// Keep the existing explicit normal cleanup sequence outside that catch.
return 0;
```

`RunMainLoop` should visibly retain pending-scene handling and its timer reset, zero-size continue, ordinary timer/input update, delta sanitization, notification update, input dispatch, game/world/audio updates, then `RenderFrame`. This is a consistent level of runtime operations; extracting every call into another wrapper would obscure it.

For `InputSystem`, keep the collection and dispatch phases in `Update`; add only private key-rule helpers proposed in step 6. Do not introduce a frame-evaluation object or move the two local maps into persistent state.

## 5. Ordered implementation steps

Each step must be independently reviewable. Compile after each code-bearing step; run the relevant focused checks below. Risk describes behavior sensitivity, not implementation size.

### Step 0 — Establish a fresh baseline (no source edits)

- Re-read this plan and the referenced functions against the actual checkout. Record revision, existing modifications, compiler, build failures and runnable tests before editing. Assign one subagent to lifecycle-order verification and another to input behavior/test verification.
- Use the commands in section 6. Separate current source builds from existing executables. Do not repair the obsolete test API or any external build blocker as part of this refactor.
- Before input edits, save event traces from the baseline for any additional characterization cases, then run the identical cases against the refactor. Keep any temporary harness/source copies under Runtime and uncommitted; distinguish those from production sources. For lifecycle cases, record debugger breakpoints/callback traces when executable, or an explicit ordered call/branch comparison when not. Run lifecycle cases in separate processes; cap retry scenarios by requesting quit after a fixed number of failures/frames so retained requests cannot hang the check.
- If a relevant suite is blocked, record the exact blocker and use the allowed source-review/temporary-characterization route. Do not silently reduce validation or claim a blocked suite passed.
- Risk: low; dependency: none. Gate: an explicit baseline and invariant list before any extraction.

### Step 1 — Clarify Runtime contracts and private state names

- Files: `GameContext.h/.cpp`, `IGame.h`, private `Impl` fields/usages in `GameApplication.cpp`. Public getters/virtual signatures stay compatible.
- Rename private `mySceneName` to `myCurrentSceneType`, `mySource` to `mySceneSource`, `myStarted` to `myInitialWorldStarted`, and `myHasMainThreadMouseLookAnchor` to `myHasMouseLookAnchor`. Rename all affected Runtime uses together.
- Explain that the context owns session input/world, input outlives world, accepted requests occupy a last-write-wins slot, Initialize has an immediate load point, and callback requests can be overwritten/cleared as described above. Document callback order and Shutdown's actual protected-region boundary.
- In `IGame`, describe parameter roles (`context`, delta seconds, display scene name, error message) in concise comments; names on virtual declarations may be added only without producing unused-parameter warnings in no-op definitions. Leave default callbacks empty.
- Do **not** initialize `myCurrentSceneType` as part of its rename: the current enum is uninitialized before successful scene assignment, a separate correctness concern. Do not imply ReloadScene/GetSceneType is defined before that assignment.
- Resulting responsibility: same classes, clearer vocabulary/contracts. Risk: low for names, medium for misleading comments. Preserve member declaration order, access/friendship and public signatures.
- Validate: build Runtime's project, compile public consumer/header isolation; search for stale private names; independently compare each new claim with code.

### Step 2 — Extract startup phases, retaining error boundaries

- File: `GameApplication.cpp`; functions: `Impl::Run`, new `InitializeWindowAndGraphics`, `InitializeServices`, `InitializeInputAndHostControls`, `StartGameSession`.
- Move existing contiguous regions into the target methods. Preserve pre-try/protected placement, singleton acquisition time, short-circuit graphics calls, asset check before overlay, overlay before audio, provider order, subscription order, startup flag position, and visibility/title order.
- Keep diagnostics widget setup inline in `InitializeServices`: one localized optional block is understandable without an additional presentation object. Use descriptive locals (`assets`, `audio`, `font`) and ordinary guarded blocks.
- Expand callbacks: phase check and each operation on separate lines; render pass change -> title -> notification. Keep callback bodies local to subscription sites. Preserve moved captures and the host subscription vector's lifetime.
- Resulting responsibility: Run coordinates startup, loop and termination; phase methods contain platform/service detail. Risk: high at extraction boundaries. Dependency: step 1 names; no cleanup redesign bundled here.
- Validate: compare the old/new ordered statements, calls, branches and catch membership against section 2; build; exercise successful startup, no-scene startup, and failures before/inside Initialize using section 6 when runnable. Verify missing overlay font remains nonfatal, and diagnostics controls remain gated while tonemapping/debug-camera controls remain unconditional. Report a source-order review as such, not as an executed trace.

### Step 3 — Make the frame sequence explicit

- File: `GameApplication.cpp`; functions: `Run`, `RunMainLoop`, `PumpWindowMessages`, `PrepareRenderTargetSize`, `RenderFrame`.
- First move the loop with its timer construction/prime intact. Then extract message pump, size preparation and rendering as the table specifies. Expand the zero-size continue and resize-error throw into braces.
- Keep pending-scene `if` and load-time timer reset together in the loop. Keep timer update, platform input update, elapsed read, notification update and dispatch order unchanged. Spell delta sanitization with a default zero and a finite-value guarded clamp if clearer; preserve `CU::Clamp` and its limits.
- Retain direct game -> world -> audio calls next to one another, followed by RenderFrame. Use a named `InputDeviceFrame` immediately before `myContext.myInput.Update` if it improves sampling/dispatch clarity; introduce no extra sample or delayed dispatch.
- Resulting responsibility: the loop shows every meaningful frame boundary; render method owns the existing snapshot/command sequence. Risk: high; dependency: step 2.
- Validate: compare baseline/refactor ordered frame traces using debugger breakpoints or temporary Runtime-local instrumentation, and distinguish source-order review when execution is unavailable. Cover quit in message pump versus input/game update; minimized/zero-size skip; restore/resize; scene failure/load timer reset; graphics finish false suppressing both execute and present. Check notification timer update precedes diagnostic input callbacks, so restarting it still gives full initial opacity that frame.

### Step 4 — Expose mouse-look side effects

- File: `GameApplication.cpp`; functions: `CaptureInputFrame`, new `CaptureMouseLookDeltaAndRecenterCursor`.
- Move only mouse-look logic into the named helper. Keep focus computation and keyboard copy in capture; call helper before the existing straightforward XInput block. Keep gamepad polling inline.
- Read focus from `frame.Focused` and right-button state from the already-filled `frame.KeysDown`. Do not call `GetForegroundWindow` or `IsKeyDown` again in the helper: the device sample must remain internally consistent.
- Use early return for disabled/unfocused/button-up only if it preserves anchor reset. `GetClientRect` failure must return without clearing a prior anchor. Retain all current cursor API calls/error handling and screen/client conversions.
- Resulting responsibility: CaptureInputFrame assembles device state; the helper explicitly samples and recenters the cursor. Risk: medium; dependency: step 3, though it can be reviewed separately.
- Validate: first activation zero movement; held movement; release/repress; alt-tab; disabled mouse look; reacquisition after successful scene load; disconnected/unfocused gamepad. Use manual observation/debugger for Windows API paths; do not add new error handling while extracting.

### Step 5 — Clarify scene and shutdown policy without generalizing it

- File: `GameApplication.cpp`; functions: `LoadPendingScene`, `Run`, `ShutdownServices`.
- Keep LoadPendingScene as one function with visibly separated candidate construction, failure handling, replacement/activation blocks. Rename locals `candidateWorld`, `loadContext`, `sceneName`; use `const std::string sceneName = GetSceneName(...)` to express value ownership (current temporary-bound const reference is valid but less direct).
- Add short comments explaining the construction-only recovery boundary, initial-versus-later failure, retained request retry, and final pending reset. Expand fallback-camera control flow into braces. Keep pending reads at their current execution points.
- Leave both explicit cleanup paths in Run; explain why normal Shutdown is outside the catch and why the exception path catches only Shutdown failures. Keep ShutdownServices as the existing three-call order, without generic cleanup flags, guards or destructor-driven service ownership.
- Resulting responsibility: scene ownership transition and failure policy readable in one place; exit semantics remain visible. Risk: high despite small edits; dependency: steps 1–3.
- Validate: section 2 scene/exit tables line by line; candidate failure keeps old world; initial failure reports then throws; bad_alloc bypasses failure callback; BeginPlay/OnSceneLoaded failure uses outer cleanup; callback requests preserve current overwrite/reset behavior; normal Shutdown throw is not accidentally caught and retried. Any newly discovered behavior bug is recorded, not fixed here.

### Step 6 — Simplify input key rules and state transitions

- Files: `InputSystem.h/.cpp`; functions: `Update`, `Dispatch`, subscription move/reset operations, key-binding predicates.
- Add private methods with these responsibilities/signatures: `static bool IsKeyBindingPressed(const KeyBinding& binding, const InputDeviceFrame& frame)` and `bool HasPressedMoreSpecificBinding(const KeyBinding& binding, const InputDeviceFrame& frame) const`. A local `.cpp` `IsKeyDown(frame, key)` function provides the existing range-checked lookup used by the first predicate. This is a real shared safety rule, not a forwarding wrapper.
- `IsKeyBindingPressed`: primary key down; all required modifiers down; no forbidden modifier down. Use explicit loops/early returns. Invalid required modifiers fail; invalid forbidden modifiers do not block, as now.
- `HasPressedMoreSpecificBinding`: iterate existing key bindings; reject different key, non-larger required count, or unpressed candidate; explicitly check every base required modifier occurs in candidate required list; return true for the first match. Do not normalize/deduplicate modifier vectors, change count semantics, sort bindings, or restrict suppression to the same action.
- Update's key loop becomes a clear pressed check followed by a suppression check **only when the base binding is pressed**. This short-circuit is the helper's precondition: checking the candidate primary key is equivalent to the old modifier-only candidate check because candidates have the same key as that pressed base. Do not eagerly evaluate both helpers. Keep focused collection, device order, maps, name registration, lexical action ordering and phase dispatch in Update, separated by short phase comments. Expand phase branches with braces and names such as `hasValue`/`wasActive` only where helpful. Do not replace the variant or combine phases into a generic evaluator.
- Expand subscription move assignment/constructor bodies and listener struct fields. In Dispatch rename `count` to `listenerCountAtDispatchStart`; retain the existing local cleanup lambda, normal/exception finalization and registration-order traversal. Explain deferred removal; do not copy callbacks or change vector storage as a readability edit.
- Resulting responsibility: key precedence readable as a named domain rule; action state machine remains local. Risk: high; dependency: step 0 characterization, otherwise independent of application steps. Can be delegated to one file-owning agent and integrated separately.
- Validate: existing `InputSystemSemantics`; additional comparison cases in section 6. Check stored state still changes after callback, no accidental value reset, no extra Ended events and unchanged removal timing. Compare temporary standalone old/new input traces when existing tests do not cover a rule.

### Step 7 — Expand dense shortcut and timer code; final integration review

- Files: `InputSystem.cpp` default bindings/action definitions; `GameApplication.h` notification timer; `GameApplication.cpp` notification early return if still compressed.
- In `InstallDefaultInputBindings`, put meaningful bindings on separate lines and replace the dense light tuple loop with three explicit small groups in the same function: directional, point, spot. Preserve each group's binding insertion order and the order SHIFT, LSHIFT, RSHIFT. Directional light: regular 7 only, plus its shifted forms; Numpad7 stays assigned to Unreal tonemapper. Point/spot retain keypad and regular keys, unmodified forms forbidden by any shift, then each modified keypad/regular pair. Do not generate all plain groups first and shifted groups later.
- Keep the tonemapper action/enum pairing loop: it is a straightforward small mapping once its callback is expanded. Do not replace it with three duplicated subscription implementations. Split unusually long action declarations into readable lines without changing names/values/definition order.
- Expand notification `Update` using named nonnegative delta and remaining time while preserving the exact nested `(std::max)` operand order and Windows macro-safe spelling. Do not replace with a different clamp expression for non-finite inputs. Keep the current opacity ternary if clear; retain duration 2.0 and fade 0.5. Simple accessors can remain one line.
- Resulting responsibility: shortcut policy and timer arithmetic obvious at the point of definition. Risk: medium (binding order/floating edge cases); dependency: integrate after relevant input/application changes.
- Validate: exact binding table comparison, input suite, timer boundary checks, public header isolation; full compile/tests as available in both Debug and Release. Final independent readability/simplicity, behavior and scope audits against the integrated diff, using the checklist below.

## 6. Validation plan and current limitations

No builds or runtime tests were executed to author this document. Findings above come from source inspection; the commands below are implementation-time validation, not claims of passing results.

From repository root in PowerShell, using the installed Visual Studio 18 Community MSBuild (v145 projects, C++20, x64):

```powershell
$runtimePlanMSBuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
& $runtimePlanMSBuild AGP.sln /m /p:Configuration=Debug /p:Platform=x64
& $runtimePlanMSBuild Tests\GameFramework\GameFrameworkTests.vcxproj /m /p:Configuration=Debug /p:Platform=x64
& $runtimePlanMSBuild Tests\GameFramework\PublicGameplayConsumer.vcxproj /m /p:Configuration=Debug /p:Platform=x64
& .\Tests\GameFramework\RunPublicHeaderIsolation.ps1 -Configuration Debug -MSBuild $runtimePlanMSBuild
& .\Bin\Tests\Debug\GameFrameworkTests.exe
```

Check exit status after each command; execute a test binary only after its current-source build succeeds. If the broad solution is blocked, attempt the direct framework project with the repository root supplied as `SolutionDir` and record any dependency blocker rather than editing it. Repeat the successful relevant checks with Release at final integration. No test requirement authorizes edits outside Runtime.

`GameFrameworkTests.cpp:355–421` covers phases, modifier handling, deferred listener removal, registration order, gamepad axis and mouse values, multiple bindings, focus loss, callback-throw cleanup, and token lifetime. `FrameTimingAndInput` checks world/input interaction. `TextGeometryAndNotificationTiming` includes notification opacity/fade/restart/expiry assertions at lines 544–554. `PublicGameplayConsumer.vcxproj` and `RunPublicHeaderIsolation.ps1` check public headers without backend imports. These do not fully verify application startup, graphics or scene transitions.

**Known runtime-suite blocker:** `GameRuntimeTests.cpp:84` calls missing `GetSceneName`; lines 100/110/140/263/286 pass strings to enum-based `LoadScene`; source lambdas at 372/381/402 take strings although `SceneSource` now takes `SceneType` (`Scenes/SceneData.h:150`). Even `camera-controls` and `render-pass-controls` modes compile through that stale translation unit. A fresh build is expected to fail until a separately authorized test migration. Do not restore a string API in Runtime to make obsolete tests compile.

When the checkout has a compatible runtime suite, build `Tests\GameFramework\GameRuntimeTests.vcxproj` and run each scenario in a separate process from repository root: `sample`, `chest-materials`, `text-overlay`, `camera-controls`, `render-pass-controls`, `invalid-initial`, `initialize-failure`, `begin-failure`, `update-failure`, `component-failure`, `shutdown-failure`. Inspect D3D diagnostics when available. At this baseline these are intended coverage, not runnable acceptance gates. Never use an older executable as evidence about the refactor.

For gaps, use read-only operation-order review, manual smoke tests on a freshly built Game, and a temporary characterization harness **under Runtime** if executable evidence is needed. Invoke the compiler explicitly so no project/test edits outside scope are required; remove temporary harness source before delivery. Prefer existing checks where sufficient. A graphics/application harness may require the existing project link dependencies and a functioning Windows/D3D/content/audio environment; if unavailable, report that exact validation gap. Do not pretend source review replaces executable lifecycle coverage.

For input comparison, compile one unchanged temporary harness with baseline `InputSystem.cpp` before editing; save action name, phase, variant index, value and event order. Recompile that same harness with the refactored source and compare those traces, including exception cases. No checkout rollback or old-header substitution is necessary. Avoid unsupported reentrant update/reset or callback-time vector mutation cases. For application order, use debugger tracepoints at the existing operations, then at their corresponding extracted call sites. The current `Source/Application/Game/Main.cpp:31–32` already enables diagnostics and mouse look, so ordinary window/input/diagnostics smoke checks need no external source changes.

| Area | Additional cases / expected result | Method |
| --- | --- | --- |
| Key specificity | Plain vs larger required list without relying on forbidden modifiers; unrelated required sets; different keys; same-count duplicates; invalid modifiers. Suppression exactly matches old predicate. | Temporary standalone InputSystem harness comparing baseline/refactor event traces. |
| Action values/order | Reverse registration/name order still dispatches lexically; same action across device categories uses last nonzero write; Ended retains previous variant value; Reset/ClearBindings emit no events. | Input harness; existing tests for covered portions. |
| Listener cleanup | Later token reset still runs current event; next event omits it; throwing callback finalizes removals and does not commit action state. | Existing tests plus targeted trace. Do not add unsupported reentrancy promises. |
| Startup/scene | No pending initial scene; valid initial/reload; failed candidate retries; initial failure fatal; source/ConfigureWorld changes pending; BeginPlay/loaded callback request discarded; bad_alloc and callback failures follow documented boundaries. | Compatible lifecycle harness or debugger/callback trace, plus independent operation-order review. |
| Frame/window | Message quit versus callback quit; scene-load time reset; zero dimensions; resize reset-before-resize; finish=false; focus/mouse anchor paths. | Windows smoke/debugger; source-order review for unforceable graphics failures. |
| Diagnostics | Missing font logs/continues; F5/F6 title and transient overlay; diagnostics disabled; tonemapping controls; timer 0/negative delta, 0.5/2.0 boundaries and non-finite parity. | `TextGeometryAndNotificationTiming` (timer assertions at 544–554), temporary timer harness for gaps and manual UI. |
| Teardown | Protected failure calls Shutdown even before Initialize; nested Shutdown failure caught; normal Shutdown failure escapes and skips explicit services; world EndPlay precedes locator/audio/assets teardown when reached. | Compatible lifecycle harness and explicit code-path audit. |

## 7. Explicit non-goals and separate follow-ups

- No ownership redesign, persistent PImpl conversion/removal, new managers, service injection framework, renderer scheduling changes, async scene loading, or public API migration.
- No universal shutdown helper/RAII cleanup policy. The current asymmetry and early initialization cleanup gaps deserve separate correctness work with broader tests.
- Do not initialize the current-scene enum, consume failed requests, retain loaded-callback requests, or snapshot the request at scene-load entry. These would change behavior or repair bugs, not just clarify it. Record them as follow-ups without treating indeterminate enum reads as a behavior to test/preserve.
- No automatic subscription clearing, callback vector redesign, recursive dispatch support, device combination policy, modifier normalization, input deadzone changes, cursor error-handling changes, minimized-window throttling, or new focus behavior.
- Do not move `RenderPassNotificationTimer` into a new file just to purify a header; it is small and externally testable. Do not remove the font friend bridge or reorder member declarations.
- Do not edit the obsolete runtime tests, `SceneData.h`, project files, sample game, renderer, services, world/component implementations or external docs to complete this work. A blocked dependency is a reported limitation, not permission to expand scope.

## 8. Independent audits and revision record

Three subagents independently investigated current code, then each received the complete draft for a second, critical review. One audited behavior/architecture; one audited simplicity and scope explicitly; one audited readability and implementation readiness. They did not edit code or this plan. The main agent checked their findings against source and resolved the following challenges.

| Review perspective | Challenge / finding | Decision and revision |
| --- | --- | --- |
| Readability | `InstallHostInputBindings` hid device configuration and subscription installation. | Renamed target to `InitializeInputAndHostControls`; its responsibility explicitly includes adapter setup, defaults and callbacks. |
| Readability + simplicity | `InitializeGameAndWorld` understated window visibility/foreground/title effects. The simplicity reviewer suggested leaving those blocks in Run or naming the full session phase. | Chose `StartGameSession` with its full contract in the target table. Retains one coherent startup phase and avoids both a tiny show-window wrapper and low-level window detail in Run. |
| Behavior + readability | Mouse helper could silently re-query focus or right-button state instead of using the sample collected before it. | Added explicit `frame.Focused`/`KeysDown` preconditions to target structure and step 4; prohibit resampling. |
| Simplicity/input equivalence | Candidate key predicate now tests its primary key too; equivalence depends on only checking suppression after the same-key base binding is active. | Step 6 now states the short-circuit precondition and explains the equivalence. No extra key-evaluator object or generic predicate framework. |
| Behavior + implementation readiness | Old/new trace comparison is underspecified if baseline traces are not captured until after edits; retained scene failures can retry forever. | Step 0 now requires pre-edit traces, separate lifecycle processes and bounded retry cases. Steps 2/3 distinguish source-order review from actual execution. |
| Readability | A true return from size preparation could be mistaken for “resized.” | Contract now says true means proceed with frame, even if dimensions were unchanged. Rendering contract explicitly includes snapshot build and conditional presentation. |
| Simplicity/validation | Existing timer coverage was vague despite concrete assertions. | Named `TextGeometryAndNotificationTiming` and exact current lines; temporary timer cases address only missing coverage. |
| Scope | Test repair/new source files could indirectly require forbidden project/API edits. | Reviewer found no required external edits. Retained explicit existing-file implementation, Runtime-local temporary harness option, and separate reporting of stale test/build blockers. No compatibility shim or project repair. |

Investigators also disagreed on whether to extract diagnostics widget setup and combine service/input initialization. The chosen target keeps the modest optional widget block inside `InitializeServices` and input subscriptions in their own substantial phase. This makes asset -> overlay -> audio -> publication order visible, without another wrapper or a combined service/input method that immediately delegates most of its work. The small tonemapper mapping remains; the denser light-shortcut tuple loop becomes explicit policy blocks.

All reviewers agreed the scene transition and the two shutdown paths should remain explicit, the nested Impl/context/input-token architecture should remain, and the obsolete runtime tests must not be “fixed” by changing Runtime's API. Audit acceptance concerns the plan's source-grounded design, not a claim that the future implementation or test suite has passed.

## 9. Final implementation checklist

- [ ] Read the current code and revalidate this plan's baseline; record pre-existing changes and build blockers.
- [ ] Assign subagents isolated lifecycle/input investigations and independent review roles; retain one integration owner and avoid concurrent edits to the same file.
- [ ] Build fresh source and establish available baseline checks; do not trust old binaries/logs.
- [ ] Keep all committed changes inside Runtime and all public signatures compatible; no project changes/new production translation units.
- [ ] Clarify names/contracts without initializing the scene enum or changing declaration order.
- [ ] Extract startup phases without moving the `try` boundary or singleton acquisitions.
- [ ] Preserve every frame operation, branch, timer update and render finish condition in section 2.
- [ ] Expose mouse cursor side effects without changing focus/anchor/API failure behavior.
- [ ] Preserve all scene pending reads, construction catch boundaries, replacement order and final pending reset.
- [ ] Keep normal and exception shutdown semantics visibly distinct.
- [ ] Verify key specificity, last-value precedence, lexical action order, callback/state order and deferred removal before integrating input changes.
- [ ] Keep shortcut binding order and notification arithmetic unchanged; avoid unnecessary wrappers.
- [ ] Compile/test incrementally and perform final Debug/Release checks where available; record blocked/unexecuted cases explicitly.
- [ ] Have independent reviewers challenge readability, simplicity, behavior/architecture and scope against the final diff. Resolve disagreements against source and evidence.
- [ ] Remove temporary harness sources, review changed paths, and provide a final report of changes, tests, limitations and deferred correctness concerns.
