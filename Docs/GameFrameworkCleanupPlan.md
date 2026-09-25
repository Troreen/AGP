# GameFramework cleanup plan

## In plain language

**The highest priorities are code readability and simplicity.** Someone new to the project, including a junior developer, should be able to find the relevant code and understand what it does. Use descriptive function and variable names, straightforward C++, and spacious formatting that makes each step easy to follow. Prefer a few clear statements over a compact expression that needs decoding.

`GameApplication` currently does too many jobs: it runs the game loop, manages the window and input, prepares scenes, connects settings to services, and draws debugging information. We should give those jobs clear owners so the application reads like a short set of instructions: start up, run each frame in the right order, and shut down safely.

Both reference projects show how a short application can delegate complete jobs. The newer reference provides a simple startup and game loop; the more mature game built on older TGE separates choosing a scene, loading its contents, switching gameplay, and showing a fade. For AGP, one scene owner should handle the current world and pending switch, while game content choices and rendering keep their own owners.

First reorganize the behavior we already have, in small steps with checks after each step. Keep the current scene running when preparing its replacement fails. Fades, background loading, and menu stacks can be considered later if the game needs them; they are separate features. This document is a plan only; it does not implement the cleanup.

## Basis and scope

- Planning branch: `gameframework-cleanup`, created from `integrate-imgui`.
- User motivation: the application felt clean and easy to follow before JSON settings parsing was added. Recover that ease of reading while keeping the current settings behavior. This is the user's recollection and desired direction, not a verified claim about repository history.
- AGP discovery found `Source/Application/Game/GameApplication.cpp` at 730 lines and its header at 109 lines. These measurements explain the concern; reducing line count is not an acceptance criterion.
- Reference inspected: `C:/Users/tarik.bergstrom/Perforce/tarik.bergstrom_TGA-SP85_7402/Source/Game/source/Go.cpp` (82 lines). The actual path includes `source` beneath `Game`.
- Additional reference: `C:/Users/tarik.bergstrom/Perforce/tarik.bergstrom_TGA-SP85_MainStream_235/Source/Game/source`, focusing on scene management, transition sequencing, and their relationship to the game loop. Its older TGE APIs are evidence for ownership decisions, not an API migration target.
- Findings below come from source inspection by discovery agents. No builds, tests, or runtime checks have been run for this planning exercise.
- No applicable `AGENTS.md` was found in the repository/reference tree or checked ancestor paths.

The scope is responsibility and lifetime separation around `GameApplication` and its existing GameFramework integration. Preserve concrete `Game` hooks, synchronous scene loading, World/Actor/Component interfaces, and the renderer interfaces. Async loading, an ECS redesign, a new global manager system, and gameplay changes are outside this cleanup.

## Governing design priorities

Readability and simplicity govern every design choice, within the requirement to preserve behavior. The proposed owners below are a means of making the code easier to navigate and understand. Their count, the number of files, and the length of `GameApplication` are not targets.

- **Write for a newcomer.** A junior developer should be able to follow startup, the frame loop, a scene request, settings application, and shutdown from descriptive entry points. Keep the relationship between a call and the code it reaches easy to see.
- **Use names that explain intent.** Functions should say what they do; variables should identify what they hold and, where relevant, which stage they belong to. Prefer names such as `requestedSceneId` and `candidateWorld` to abbreviations or generic names that require surrounding code to decode.
- **Give the code room.** Use blank lines between logical steps, clear blocks, and one meaningful action per statement where practical. A longer, airy function can be easier to read than a dense chain of expressions. Do not introduce arbitrary line limits or compress code to make the application look smaller.
- **Use straightforward C++.** Prefer ordinary control flow, named intermediate values, and explicit operations when they explain the work clearly. Use C++20 features when they provide a concrete readability or correctness benefit; avoid clever syntax, template machinery, or dense callback expressions introduced merely to modernize the code. Keep appropriate existing lifetime and ownership tools.
- **Make each extraction earn its place.** Extract a coherent job when it helps readers find it, understand its state, or see its lifetime. Keep small related steps together when splitting them would force readers to jump through wrappers. Prefer a well-named function or a concrete owner before adding interfaces, generic frameworks, or extra layers.
- **Keep necessary complexity visible.** Preserve the explicit failure and cleanup order described below. Clear names, spacing, and short comments about reasons should make those constraints understandable; hiding them behind vague helpers does not make the code simpler.

When two behaviorally equivalent designs compete, choose the one a newcomer can explain more easily. A smaller diff, fewer lines, greater generality, or newer syntax does not outweigh that goal. Apply this guidance to the code touched by the cleanup without expanding it into an unrelated repository-wide style rewrite.

## What to take from the reference

`Go()` performs registration and settings setup, starts Application and GraphicsEngine, initializes a scoped GameWorld, then visibly sequences BeginFrame, update, render, EndFrame, and shutdown. Window/DX11/ImGui/filewatcher work belongs to the reference Application; rendering resources belong to GraphicsEngine; gameplay belongs to GameWorld.

Our equivalent should expose that same readable lifecycle and clear ownership. We should not copy its singleton pattern or timing behavior. The reference is a simpler 2D flocking example, and its GameWorld still contains substantial gameplay/debug UI code (391 lines). Its startup failure path also returns before explicit shutdown. AGP's existing cleanup and scene failure guarantees must survive the refactor.

Existing useful AGP boundaries should remain in place: `Game.cpp` owns game hooks/content; `GameWindowMessages` handles the Win32 pump and ImGui input capture; `WindowSettings` and `InputSettingsApplier` apply settings; `WorldRenderer` adapts a snapshot for rendering; and importing already flows through SceneData, BuildWorldFromSceneData, and World.

### What the more mature game adds

Paths in this subsection are relative to `C:/Users/tarik.bergstrom/Perforce/tarik.bergstrom_TGA-SP85_MainStream_235/Source/Game/source`. The following are source observations, not evidence that this checkout builds or that every feature works.

| Reference responsibility and evidence | Lesson for AGP |
| --- | --- |
| `Go.cpp:32-75` runs engine startup, a scoped GameWorld, the frame loop, then engine shutdown. `GameWorld.cpp:219-315` manages startup and routes menus/gameplay through states; `StateStack/State.hpp:64-74` owns scene objects, renderer and camera references. | A short entry point is enabled by ownership below it. Keep AGP's application, game hooks, and World responsibilities distinct; menu/state stacks are not required for this cleanup. |
| `SceneManager.h:9-36` and `SceneManager.cpp:83-107` contain catalog/current-path bookkeeping and a single pending path. They do not own the scene objects or their lifecycle. | Separate content lookup from the owner that actually replaces the World. Calling an object SceneManager does not establish its responsibilities. |
| `StateStack/InGame.h:34-37` owns transition/fade alongside gameplay systems. `StateStack/GameState_InGame.cpp:185-215` wires application callbacks; `:351-354` processes transitions after gameplay/object iteration. | Give switching a named owner and a safe checkpoint. Preserve AGP's existing checkpoint before update; copying the older game's end-of-update placement would change request timing. |
| `SceneTransitionController.h:24-32` defines a request; its `.cpp:79-145`, `:263-355` coordinates preparation, fade-to-black, scene application, fade-in, and a latest queued request. `SceneLoadingService.h:15-28,47-63` separates parsed data from live object construction. | Keep request coordination distinct from candidate construction and visual presentation. AGP already has SceneData and World construction; reuse those boundaries with synchronous execution. |
| `StateStack/GameState_InGame.cpp:422-446` draws the fade after scene/UI; `:142-153` shuts down transition work, resets fade callbacks, and unregisters a listener. | Presentation belongs to rendering; callback and work lifetimes must end before their targets are destroyed. AGP's cleanup ordering remains explicit. |

The reference's runtime transition flow prepares plain scene data on a worker, waits for it, fades to black, builds live objects on the main thread, applies them, and fades in. Boot loading is synchronous. It can preload one unambiguous authored next-scene target and reuse ready data or a pending future. This demonstrates useful separation; it does not establish that AGP's importer, asset registry, or configuration hooks are safe on a worker thread.

It also has two request routes: the SceneManager path mailbox and typed requests through WorldTransitionService. The immediate door route drops authored spawn/fade metadata (`LevelTransitionDoorComponent.cpp:79-95`). The transition controller suppresses current-scene requests unless forced and queues the latest distinct target while busy. AGP must retain its own last-write-wins slot and same-scene reload behavior. If richer requests are later needed, extend one request boundary rather than adding competing routes.

Several details should not become AGP requirements:

- The older `StateStack/State.cpp:78-124` publishes scene identity and clears nonpersistent old objects before validating/initializing the replacement. Its persistence policy differs from AGP's complete World replacement. Keep AGP's stronger candidate-before-commit guarantee and post-commit BeginPlay semantics.
- `SceneImportService.cpp:42-51` returns an empty collection on load failure, and LoadedSceneData lacks explicit success status. Boot loading reports success after applying its result (`SceneTransitionController.cpp:63-71`). Retain AGP's existing diagnostics and failure handling; an empty scene must not silently stand in for failure.
- The reference accepts `targetSpawnId` but discards it in `StateStack/GameState_InGame.cpp:472-481`. Spawn selection is not a demonstrated feature to reproduce.
- `SceneTransitionController.cpp:151-169` waits for pending work rather than truly cancelling it; future retrieval has no local catch at `:136,419`. Its fade-busy path can drop prepared work (`:289-293`), and fade completion calls out before clearing its stored request (`ScreenFadeController.cpp:83-87`), creating a reentrancy hazard when a queued transition starts immediately. Any future asynchronous/fade design needs explicit completion and callback lifetime rules; none of this machinery is needed for the synchronous extraction.

### Optional follow-on work, outside this cleanup

Only take these on as separately scoped gameplay requirements after the cleanup is verified:

- **Fades/loading presentation:** a presentation controller may observe transition progress, but the scene session stays the only owner allowed to replace the World. Define update/input behavior during a fade, overlay order, failure recovery, and exactly-once completion; clear old callback state before invoking callbacks that can start another transition.
- **Background parse/preload:** first prove the chosen parsing path is worker safe. Keep assets/live World construction on the appropriate thread, define structured load outcomes, and discard stale results using request identity. Define shutdown waiting/cancellation and cache invalidation before adding work. Do not inherit the older reference's one-target preload policy automatically.
- **Menus, persistent actors, spawn destinations, or richer queues:** each changes game behavior and needs its own lifecycle contract. If typed requests gain spawn/fade/reload fields, preserve their complete intent through every producer and define request equality explicitly.

## Proposed responsibility boundaries

Names below describe proposed collaborators and can be adjusted during implementation. Each should own a coherent job and expose a small interface; avoid extracting a collection of helpers that still share all application state. This table maps responsibilities rather than prescribing one new class per row. Combine closely related responsibilities or use named functions when that produces a clearer path for the reader.

| Owner | Responsibility | Placement and boundary |
| --- | --- | --- |
| `GameApplication` | Compose owners; make startup, frame order, scene checkpoints, and shutdown order explicit; order graphics initialization before service assets | Application layer; retain direct `Game` lifecycle hooks; avoid becoming a service/state bag |
| Window host | Window/class lifetime, cursor/mouse state, existing window message handling, window settings, raw-input lifetime | Application layer; reuse `GameWindowMessages` and settings helpers |
| Runtime services and settings bindings | Establish service dependencies, apply settings, own subscriptions and their teardown | Application layer; explicitly model callback lifetimes and partial initialization |
| Frame renderer and diagnostics | Own the application's `GraphicsCommandList`, snapshot/render/present work, render settings/listeners, debugging widget/font resources; encapsulate graphics/debug UI initialization calls | Application layer; reuse `WorldRenderer`; singleton `GraphicsEngine` retains its existing engine resources; coordinator explicitly orders initialization and debug UI shutdown before `KillServices` |
| Scene session / transition owner | Sole owner of current World, current scene identity, request acceptance and pending request; coordinate candidate preparation and commit at the application checkpoint | Start in application layer; retain `GameApplication` request/reload forwarding methods; move reusable pieces to framework `Scenes` only if inputs/callbacks exclude `Game`, game scene IDs/catalog, and `PrimitiveMeshBuilder` |
| Scene preparation | Resolve supplied content, import SceneData, obtain supplied fallbacks, build a candidate World, and run supplied game configuration | A cohesive operation behind the scene session boundary initially; keep the catalog lookup and preparation exception boundary distinct; introduce a separate loader class only if it earns independent ownership or reuse |
| Game scene catalog and fallback provider | Map game scene choices to content and construct game-specific fallback content | Game/application layer; do not put game content policy into GameFramework |

Clear ownership supports the readability and simplicity priorities. Prefer explicit references for dependencies and one clear owner per resource. Any callback into another owner must have a documented lifetime and teardown point. Keep ordering visible at the composition root rather than hiding it in constructors/destructors with implicit dependencies.

## Behavior that must remain intact

### Startup and scenes

- Settings load in `Main` before window/services creation. `Game` owns MeshLibrary, whose lifetime keeps the FBX importer alive.
- Requests can be made during `Game.Initialize`; pending scene requests are last-write-wins and are consumed at frame boundaries. Apply the startup default only when there is no pending request.
- Consume the pending slot before preparation. Requests raised by configuration, old-world cleanup, or BeginPlay callbacks remain pending for the next checkpoint; do not clear them when the current switch finishes or recursively execute them. Reload targets the committed current scene and returns false before a scene exists or after requests are disabled. A request for the current scene still reloads it; do not add duplicate suppression.
- Prepare/import/build a candidate, including fallback handling and `Game.ConfigureWorld`, before replacing the current world. Only `std::exception` failures caught within candidate preparation are recoverable after startup, with `std::bad_alloc` excluded and always fatal. Startup load failure is fatal. Scene catalog lookup is outside the preparation `try`; failures there and non-standard exceptions remain fatal.
- Commit clears the old world, sets scene ID, resets/ensures the debug camera, then invokes BeginPlay. Failures in clear/debug camera/BeginPlay after preparation remain fatal. BeginPlay is **after commit** and is not protected by rollback. The cleanup must not accidentally advertise or implement a broader transaction.
- Refresh the timer after loading. Frame delta remains finite and clamped to 0–0.25 seconds.
- Process the scene checkpoint before testing for a minimized or zero-size window, as today. A component callback may request a later switch; it must never synchronously clear the world during `World.Update` or `World.BeginPlay`.

### Frame and input

Preserve the observed sequence: message pump/settings/scene checkpoint → resize → input → quit check → BeginDebugUi/F9 → camera toggle → `Game.Update` → `World.Update` → audio → debug UI → snapshot/render/present. Minimized or zero-size windows skip input, update, and rendering as they do today.

`GWLP_USERDATA` holds a pointer to InputHandler. That object's address and lifetime must remain stable while the window can dispatch messages. Moving input ownership must preserve this invariant and the existing ImGui capture rules.

Resize must reset the command list before graphics resize, then update world camera resolution. If `FinishCommandList` fails, call `ImGui::EndFrame` and skip command execution, `RenderDebugUi`, and presentation for that frame.

### Shutdown and failure handling

Reject new scene requests → `Game.Shutdown` (including partially initialized games) → clear world → application listeners/mouse cleanup → snapshot/font/widget references → debug UI → `ServiceLocator.KillServices` → window cleanup.

Keep the existing AttemptCleanup behavior for its protected application steps: retain the first failure while attempting subsequent protected steps. `ShutdownDebugUi` is currently called directly, and `KillServices` is one aggregate cleanup step; this is not a guarantee of recovery from every individual resource failure. Settings callbacks currently capture the application and last until settings service deletion. Extraction must remove or rebind those callbacks before their target collaborator is destroyed; moving code alone does not make callbacks safe.

## Migration plan

Every phase must pass a readability review as well as its behavior checks: the changed flow should use descriptive names, spacious formatting, straightforward control flow, and a navigation path a newcomer can follow. Explain what each new boundary makes easier to understand. Revise or combine an extraction that adds indirection without that benefit, even if it reduces `GameApplication`'s line count.

### 1. Establish the baseline and contracts

Record the current startup/frame/shutdown order and the ownership map above against the implementation. Include the scene request/checkpoint/preparation/commit contract and explicitly distinguish it from the older reference's fade/queue policy. Build the Game target and existing framework tests before edits, recording pre-existing failures separately. Review the existing scene, input, and lifecycle coverage; add focused behavioral tests or an appropriate lightweight seam only where the refactor introduces concrete risk.

Existing `GameApplicationTests.cpp` covers camera controls and legacy `GraphicsEngine::SelectPrevious/NextRenderPass`, not the actual application's `myRenderSettings`/listener behavior. Its project compiles `GameApplication.cpp` but appears to omit `GameWindowMessages.cpp`; establish whether this causes a baseline link failure before relying on that test target. These are source findings, not confirmed build failures.

Exit: baseline results and a short checklist of observable behavior are recorded; validation can distinguish old failures from regressions. Record where a newcomer would start to trace startup, scene loading, settings application, a frame, and shutdown so later reviews can assess whether navigation has improved.

### 2. Separate scene policy and scene transitions

Implement this phase in three reviewable steps:

1. Extract the game scene catalog/fallback provider first, including the game-owned scene identifier definition. Keep the existing `RequestSceneLoad(SceneId)` and `ReloadCurrentScene()` entry points on `GameApplication` as forwarding methods so `Game.Initialize` and its input bindings retain their contract.
2. Move the current World, current scene identity, pending request, request acceptance, and initial-scene success bookkeeping into one scene session owner. Expose current-world access for the coordinator's existing update/render calls and a single operation to process a pending request at the existing checkpoint. A load attempt should report enough information for the coordinator to preserve timer refresh after both successful and recoverably failed attempts. Do not maintain duplicate pending/current state in the application.
3. Move candidate preparation and commit behind that owner. Supply game configuration and game fallback policy explicitly; retain SceneData, UnrealSceneImporter, BuildWorldFromSceneData, and World. Keep catalog lookup outside the recoverable preparation block. Commit still clears the old world, installs the candidate and identity, resets/ensures the debug camera, then runs BeginPlay. Treat camera integration as an explicit application dependency or narrow callback, not a reason to pull graphics/platform state into a generic scene API. The existing debug camera service must have one owner and survive every call that uses it.

Use ordinary synchronous operations for consume, prepare, and commit. They describe the lifecycle without requiring an asynchronous state machine or a second transition manager. Stop accepting requests before `Game.Shutdown`, but retain the current World until the existing world-cleanup step. Keep the timer in the loop coordinator and screen drawing in the renderer.

Only promote the generic part into framework `Scenes` when its public API can remain independent of application/game types and graphics/platform headers. Keeping it application-local is acceptable if that avoids a premature abstraction.

Exit: exercise requests during initialization, multiple requests in one frame, successful switching, reload of the committed current scene, and same-scene requests. Check requests raised during preparation/commit survive to the next checkpoint without clearing a world during its callbacks. Exercise recoverable preparation `std::exception` preserving the old world and scene identity, and fatal startup/allocation failures. Verify the catch boundary stays unchanged for catalog lookup, non-standard exceptions, and post-preparation clear/debug camera/BeginPlay failures. Check camera reset, timer refresh after attempted loads, scene processing while minimized, and request rejection during shutdown.

### 3. Separate frame rendering and diagnostics

Move the application command list, render settings/listeners, snapshot/font/widget responsibilities, and graphics/debug UI initialization calls to the frame renderer/diagnostics owner. Keep singleton GraphicsEngine ownership intact. Leave frame sequencing in `GameApplication`, with explicit calls where debug UI begins, game/world/audio update occurs, and rendering finishes. The coordinator must initialize graphics before service assets and shut down debug UI before `KillServices`. Keep snapshot references cleared before destroying the services/resources they depend on.

Exit: verify initial rendering and rendering after scene changes, F9/render-mode controls, camera/debug UI interactions, and startup/shutdown resource order. Verify resize resets the command list before graphics resize and then updates world camera resolution. Exercise failed `FinishCommandList`: ImGui ends the frame, and execution, debug UI rendering, and presentation are skipped. Tests of the legacy GraphicsEngine render-pass API alone do not satisfy this phase.

### 4. Separate window/input and service/settings lifetimes

Extract the window host using the existing message and settings helpers. Then establish the runtime service/settings owner, preserving startup dependency order and explicit shutdown order. Keep InputHandler stable while attached to the window; detach bindings and remove listeners before their targets die. Make partially completed startup safe at each acquired-resource boundary.

Exit: verify quit, resize, minimize/restore, mouse capture, ImGui keyboard/mouse capture, settings changes, and partial initialization failures. Confirm requests are rejected during teardown, `Game.Shutdown` still runs when required, and a failure in an AttemptCleanup-protected step does not skip subsequent protected steps.

### 5. Finish the coordinator and document the structure

Remove obsolete application state and includes after the owners are established. Make the application header expose only necessary dependencies. Review the main loop and lifecycle methods as a reader: each call should name a meaningful operation, and the ordering constraints should be understandable without opening every collaborator. Review touched functions and variables for descriptive names, expand compressed logic where it obscures steps, and leave visual space between those steps. Remove wrappers or abstractions whose main effect is extra navigation.

Update `Docs/GameLoopOnboarding.md`, `Docs/GameFrameworkMVP.md`, and `Source/Engine/GameFramework/README.md` to explain responsibilities, scene commit semantics, and lifetime dependencies.

Document the request path from game/input callback through the application's forwarding API to the scene session's pending slot and next checkpoint. Show where catalog/preparation, camera integration, and rendering meet it. Make clear that the cleanup retains one live World and synchronous loading; the optional follow-on features above remain separate work.

Exit: no duplicated ownership, stale callback captures, or hidden ordering dependency remains from extraction; final validation below passes or identifies a documented pre-existing limitation. A reviewer approaching the code as a newcomer can find and explain the main flows, identify the owner of each resource, and follow failure handling without decoding abbreviations or compact syntax. Each phase should be a reviewable commit with a working application before the next extraction.

## Build integration and verification

- Review readability directly; builds and tests cannot establish it. Have a reviewer unfamiliar with the extraction trace startup, scene request through commit, a settings change, a frame, and shutdown using the code and brief navigation documentation. Record confusing names, unnecessary jumps, dense expressions, or hidden order and resolve them before accepting the phase. Use a junior developer's perspective; do not claim junior-developer validation unless one actually participated.
- Keep checked-in Game `.vcxproj` and `.filters` explicit source lists consistent with new files. Premake uses source globs; use the repository's generation flow coherently and avoid unrelated project churn. The hand-maintained test project must also include required translation units.
- Build the Game target in the repository's established Debug/x64 configuration after each extraction. Run relevant existing GameFramework tests and the focused regression checks affected by that phase.
- Run `RunPublicHeaderIsolation.ps1` when framework headers change. It scans World/Components/Scenes and disallows Windows/GraphicsEngine/RHI includes; a new generic scene header must meet that boundary.
- Use targeted tests for state transitions, request precedence, preparation/commit failure boundaries, and cleanup ordering where a practical seam exists. Use a runtime smoke checklist for window/input/UI/render behavior that unit tests cannot establish. Record which checks were automated versus manual.
- Finish with one startup → scene switch → settings/debug controls → resize → minimize/restore → quit smoke pass, plus the relevant failure-path checks. Do not broaden testing without a concrete remaining risk.

## Main risks and controls

| Risk | Control |
| --- | --- |
| More layers or clever syntax make the code harder for newcomers | Require a clear navigation benefit for each extraction; use descriptive names, spacious formatting, and straightforward C++; simplify or combine boundaries when that improves reading |
| Code gets shorter but ownership stays tangled | Require a named owner and small interface for each extracted responsibility; reject broad shared-state access |
| Game policy leaks into engine framework | Keep catalog/fallback/Game hooks in the application; promote only proven generic scene logic |
| Scene failure behavior changes | Test preparation versus commit separately, including BeginPlay after commit |
| Mature reference introduces unwanted behavior | Retain AGP request timing, reload behavior, whole-World replacement, and synchronous execution; scope fades, persistence, workers, and queue policy separately |
| Dangling settings/listener/input pointers | Document callback lifetimes; detach before targets die; retain stable InputHandler address |
| Teardown breaks after partial startup or an exception | Preserve explicit cleanup sequencing and first-failure retention; exercise failure boundaries |
| Project files/tests stop building after files move | Establish baseline, update source lists alongside each extraction, and run focused builds |

The desired result is code a newcomer, including a junior developer, can navigate and understand: a readable application lifecycle, descriptive names, spacious and straightforward C++, and collaborators that make complete responsibilities easier to follow, with the game's existing behavior preserved.
