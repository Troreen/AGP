# Engine foundations while scene importing is in progress

Historical foundation plan. Its Connect/OnDestroy hooks, exposed lifecycle and scene recipes are superseded by the implemented [gameplay guide](GameFramework.md) and [implementation record](SimplifiedGameFrameworkImplementation.md). Retained below for design history, not current API instructions.

## Objective and scope

Implement the runtime foundations already agreed in EngineDecisions.md without
depending on completion of the JSON parser. Keep ModelViewer runnable throughout.
Use directly constructed C++ test scenes until the importer adapter is available.

This is an implementation plan, not a claim that these capabilities exist. The
operational defaults below are proposed for this work; they must not silently be
promoted to previously agreed decisions. Record their adoption when implementation
starts, and reconcile any changes with the decision register.

Do not implement a competing JSON parser, new asset manager, action-mapping system,
audio backend, reflection system, physics integration, general event bus, pause
system or gameplay features in this increment. Those retain their own integration
work and decisions. Existing input samples and asset-loading code remain usable.

## Delivery order

| Stage | Deliverable | Dependency |
| --- | --- | --- |
| 1 | Safe lifetimes, handles and component service access | Can start now |
| 2 | Actor hierarchy and spatial component transforms | Stage 1 |
| 3 | Type registration and dependency resolution | Stage 1; can be developed before Stage 2 finishes |
| 4 | Candidate-world construction and scene replacement | Stages 1-3; description/asset contract checkpoint |
| 5 | ModelViewer driven through the builder, then importer adapter | Stage 4; parser needed only for the final adapter |

Each stage updates tests, Visual Studio project/filter entries and self-contained
onboarding comments. Source comments must not refer developers to private Docs files.
Preserve unrelated working-tree changes and keep each stage independently buildable.

## Stage 1 - Safe lifetimes and handles

### Ownership and public interfaces

- Keep World as actor owner and Actor as component owner. Heap object addresses stay
  stable while alive; pointers are short-lived borrows, not persistent references.
- Introduce ActorHandle and typed ComponentHandle<T>, with slot/generation identity
  and a weak world-lifetime token. Resolution fails for expired worlds, recycled
  slots, wrong types and objects marked for destruction. Old-scene handles cannot
  accidentally resolve into a replacement world.
- Add component Connect/validation, BeginPlay and EndPlay hooks. Keep OnDestroy as
  attachment cleanup for compatibility, separate from EndPlay.
- Add explicit world preparation, activation and shutdown operations. Construction
  alone does not start ticking. AddComponent still assigns owner/name after the
  constructor and returns a configurable object; pending additions are not inserted
  into the collection currently being iterated.
- Add Component::GetWorld() and read-only GetInput() through an attached session
  service binding. Retain the existing GameInput implementation as a temporary
  backend. Do not invent audio/asset services or finalize action-input APIs here.

### Proposed lifecycle defaults

- World states: constructing, prepared, active, ending. Object states distinguish
  pending, connected, begun and pending destruction.
- Connect all objects and validate dependencies before any BeginPlay. Invoke BeginPlay
  once in actor/component creation order, including initially inactive/disabled
  components; activation/enabled state governs ticking, not repeated initialization.
- BeginPlay may assume dependencies exist and are connected, but not that their
  BeginPlay has run. Do not introduce a dependency-order scheduler in this stage.
- Flush pending structural changes at the start of a gameplay frame, before its
  fixed steps. Changes requested in any callback become eligible next frame, not
  during later catch-up steps in the same frame. Freeze each flush batch; changes
  requested during lifecycle callbacks wait for the next boundary.
- Destruction requests immediately make objects unavailable to normal lookup and
  skip later callbacks/snapshot extraction. The executing callback may finish;
  memory remains alive until the flush boundary. Duplicate requests are harmless.
- Runtime additions are connected/validated as a batch before activation. Invalid
  additions are discarded with diagnostics and a development assertion, without
  corrupting the already active world. Destroy-before-activation never calls BeginPlay.
- EndPlay runs once for components whose BeginPlay completed. Disabling/re-enabling
  does not replay it. Teardown uses reverse activation order, then OnDestroy; lifecycle
  callbacks must not assume referenced peers remain available during teardown.
- Shutdown suppresses new structural requests, ends live objects and invalidates the
  world token. Required dependency handles becoming unavailable during play must be
  handled by consumers; they are not promises that targets can never be destroyed.

### Integration and acceptance

- Activate the initial world after IGame::Initialize and before the first snapshot.
  Join gameplay work before session shutdown, preserve the game Shutdown hook's
  access to an alive world, then finish world teardown before destroying services.
- Replace persistent raw references in ModelViewer behaviors and active-camera
  selection with handles. Move lazy owner-dependent setup to BeginPlay.
- Test lifecycle order, disabled/inactive initialization, self-destruction, removal
  of a later component during iteration, spawning in every phase, repeated destroy,
  destroy-before-start, slot reuse and handle resolution after world destruction.

## Stage 2 - Hierarchy and spatial components

### Transform ownership

- Actor retains its own local transform and optional parent actor. Introduce
  SceneComponent : Component, with its own local transform and optional spatial
  component parent. MeshComponentBase, CameraComponent and LightComponent derive
  from SceneComponent; ordinary behavior components remain non-spatial.
- Initially allow component attachment only within the same actor. A parentless
  SceneComponent inherits the actor world transform; multiple such components are
  allowed. Cross-actor placement uses the actor hierarchy.
- Actor and component attachment operations validate same-world ownership and
  reject self-parenting and cycles. Change parent links only at safe boundaries
  during play; initial scene construction can attach before activation.
- Expose explicit local transform editing and world-matrix access. Preserve the
  current row-vector rule: world = local * parentWorld. Keep existing actor transform
  access as local for compatibility, and migrate ambiguous callers explicitly.
- Reparenting requires an explicit KeepLocal or KeepWorld mode, with no implicit
  choice. KeepWorld converts through the parent inverse; fail without changing the
  object when the parent is singular or the result cannot be represented as TRS.
- Local storage remains translation/rotation/scale. Preserve full composed world
  matrices, including hierarchy-induced shear. Do not silently decompose away shear
  during world editing/import; unsupported local matrices produce validation errors.

### Activation, deletion and rendering

- Implement actor effective-active state from all actor ancestors, preserving local
  flags. Disabled spatial components do not disable their attached children by
  default; enabled state applies to that component's behavior/render contribution.
- Destroying an actor marks its actor descendants and owned components. Removing a
  spatial component marks its attached component subtree for removal. Perform
  child-first teardown and detach internal transform pointers before freeing owners.
- Use component world matrices for mesh placement, bounds/culling and shadow items.
  Cameras and lights derive world position and normalized orientation from their
  component transforms; inherited scale does not change authored light radius or
  camera projection. Keep mesh scale in its full world matrix.
- Select the active camera by a CameraComponent handle, allowing multiple cameras
  on one actor. Expired/disabled cameras produce no new scene snapshot; keep the
  host responsive and diagnose the missing camera rather than dereferencing it.
- Check every actor-transform assumption in render snapshot extraction and sample
  controls. Identity component offsets must preserve the existing scene appearance.

### Acceptance

- Test nested translations/rotations/scales, two offset meshes on one actor,
  parented camera/light direction, actor activation inheritance and subtree deletion.
- Test reparent modes, singular parents, shear rejection for TRS edits and cycle
  rejection. Verify parent removal leaves no dangling Transform parent pointers.
- Add a small visible hierarchy example to ModelViewer using existing assets; verify
  geometry, lights, cameras and shadows agree on component world placement.

## Stage 3 - Registration and dependency resolution

- Add an extensible registry under GameFramework/Scenes. Register engine and game
  types through the same typed creation helper and optional configuration callback.
  Duplicate names are errors; freeze registration before building scenes.
- Keep exporter numeric TypeIDs outside the runtime registry. An adapter will map
  them to registered runtime type names. Unknown source types must not be silently
  dropped or become arbitrary engine type IDs.
- Separate creation/attachment, configuration and connection. Resolve required and
  optional sibling/cross-actor dependencies during preparation, storing handles.
- Required type-only dependencies need exactly one match. Named selection checks
  both name and type. Missing required or ambiguous selections are validation errors;
  absent optional references are allowed, but a supplied invalid target is an error.
- Return a collection of structured diagnostics with actor/component/property
  context. Builder validation is testable without assertions; the application-level
  load boundary logs, cleans up and asserts according to E15.
- Do not finalize generic PropertyBag serialization before the importer checkpoint.
  Registry creation and dependency tests can use simple in-memory configurations
  and test components without a competing property format.
- Test duplicate registrations, freeze enforcement, unknown types, forward references,
  required/optional dependencies and multiple same-type components.

## Stage 4 - Builder and scene replacement

### Contract checkpoint

Before binding the public builder to imported data, agree with the importer team on
the concrete payload representation, component identities, parent references and
error representation. Choose the variant/polymorphic/property-bag approach jointly;
this plan does not select one on their behalf. Asset resolution remains delegated to
the existing game loader through a narrow adapter until its team-owned API is ready.

The independent work can still build candidate worlds through the registry and
validate them using C++ fixtures. At this checkpoint, add an engine-facing scene
description and the input adapter without changing lifecycle or registry semantics.

### Construction and commit

- Build into a separately owned candidate World bound to the same session services.
  Create all objects, apply settings, resolve hierarchy/dependencies, then validate.
  No BeginPlay, ticking or render publication occurs during candidate preparation.
- Require unique actor identifiers, unique component names per actor, valid parent
  links and an explicit active-camera component reference for the sample game.
- Return either a prepared candidate or diagnostics. Failed preparation releases
  candidate objects and retains the old scene; failed initial loading does not enter
  gameplay. Development assertions happen after reporting and cleanup.
- Process scene-change requests on the platform thread between rendered frames.
  Stop/join the gameplay worker before preparation and GPU resource creation. Use
  the existing synchronous resource-loading path, not worker-thread graphics calls.
- Retain the old scene and last snapshot until preparation succeeds. On failure,
  resume the old world when execution continues. On success, end the old world,
  activate the candidate, reset old snapshot state and publish the candidate's first
  snapshot before restarting gameplay. Services outlive both worlds.
- Discard pending input edges/mouse deltas across the transition and reset timing
  accumulation so loading time is not simulated as catch-up work.
- BeginPlay exceptions after commit are fatal session errors with orderly cleanup;
  do not claim rollback to an already ended scene. Never let worker exceptions escape
  without joining. Shared mesh/material assets stay immutable during rendering.
- This first synchronous path may display a static loading frame. Animated progress,
  asynchronous uploads and background preparation are separate work, not hidden
  requirements for scene replacement.

### Acceptance

- Test valid candidate activation, aggregated validation errors, candidate cleanup
  without BeginPlay, previous-scene preservation, replacement teardown and expired
  old-scene handles. Test both assertions-disabled failure returns and an isolated
  development assertion path.
- Exercise threaded and synchronous host modes, including shutdown and callback
  failure during replacement. Verify no stale camera or previous-scene snapshot is
  submitted after the new scene has been committed.

## Stage 5 - ModelViewer and importer integration

- Describe the existing camera, meshes, animated actor and lights through the agreed
  C++ scene description and registry. Keep game behaviors in the application project.
- Remove per-component GameContext constructor plumbing in favor of scoped getters;
  use required dependencies and handles instead of repeated sibling searches/raw
  pointers. Keep direct key controls as the temporary input backend.
- Preserve the existing scene/control behavior and add explicit spawn/destroy and
  reload exercises to demonstrate safe lifetime behavior. Keep these diagnostics
  outside the reusable engine's game rules.
- When imported data becomes available, add only the format adapter: translate type
  IDs, preserve materials in slot order, convert coordinates exactly once and derive
  parent-local component matrices from actor-relative exports.
- Coordinate conversion and source-data validation require the missing exporter
  convention details and a nested fixture; they are not prerequisites for testing
  the runtime with engine-space C++ descriptions.

## Completion checks

- ModelViewer builds in Debug and Release x64 after each stage; run CPU lifecycle,
  hierarchy, dependency, builder and existing scheduling regression suites as added.
- Manually verify both host modes: camera movement, animation switching, light
  controls, chest rotation, hierarchy placement, destruction and successful reload.
- Validate invalid scenes without changing the current active world, and check
  shutdown during loading/after failure without stale handles or use-after-free.
- Confirm game code contains no threading, deletion queues or render scheduling.
- Update the decision register with adopted defaults, implementation status and
  remaining team-owned work. Leave parser, asset and input integration explicitly
  pending until their contracts are available.
