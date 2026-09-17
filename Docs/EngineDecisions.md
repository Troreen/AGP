# Engine architecture decisions

This register records agreed architectural direction and outstanding decisions.
**Agreed does not mean implemented.** It is not a description of every capability
currently available in GameFramework. API names mentioned here describe intent and
may change during implementation.

## Status definitions

- **Agreed:** accepted architectural direction; implementation may still be pending.
- **WIP:** related work is underway; the integration contract is not yet finalized.
- **TBD:** a decision or team agreement is still needed.
- **Deferred:** deliberately outside the initial implementation scope.

## Project constraints

| Area | Decision / requirement |
| --- | --- |
| Game scope | A Diablo-style action-RPG vertical slice, not a full game of that scale. |
| Repository | One game in the repository at a time. Reuse the engine as the foundation of later projects. |
| Reuse strategy | General reusable core; add capabilities when the current project requires them. |
| Platform | Windows. |
| Input devices | Keyboard/mouse and gamepad. |
| Players | Single-player only. |
| Expected scale | Moderately sized scenes with approximately 200-300 actors. |
| Performance target | Above 100 FPS; reference hardware and measurement conditions remain TBD. |
| Authored data | Materials, meshes, animations and basic actor properties configurable through JSON. |
| Behavior authoring | Complex behavior implemented in C++ initially. |
| Required capabilities | Basic physics, collision detection, triggers and navigation support. Navmesh implementation comes later. |
| First playable milestone | TBD. |

The current planning focus is engine architecture. Movement, combat, root motion and
other game-specific behavior decisions are not settled by this register. Established
engines may inform individual decisions; their complete feature sets are not scope
requirements.

## Agreed decisions

### E01 - Engine/game boundary

**Status: Agreed**

- GameFramework owns reusable lifecycle, world/component infrastructure and engine-facing services.
- Game code owns its rules, custom components and content selection.
- Gameplay programmers use ordinary callbacks; scheduling, synchronization and rendering handoff remain engine responsibilities.
- Ordinary gameplay callbacks remain serialized.

### E02 - Convenient service access

**Status: Agreed**

- Attached components access common services through simple getters, such as `GetInput()`, `GetAudio()` and `GetWorld()`.
- Services are owned by the running session and reached through the component/world relationship.
- Ordinary components do not require a `GameContext` constructor parameter merely to participate in the engine.
- A basic behavior should depend only on what it uses; for example, rotation need not depend on input.
- A global class of independently owned singletons is not required. Exact service storage and getter APIs remain implementation details to define.
- Gameplay input access exposes a stable sample, not the underlying platform input handler.

### E03 - Action-based input

**Status: Agreed; integration WIP**

- Gameplay reads named actions rather than hardcoding physical device bindings.
- Keyboard/mouse and gamepad bindings are configured separately from gameplay behavior.
- An existing input-action system may be available to port. Inspect it before designing a replacement.
- Binding representation, menu input capture, rebinding and fixed/variable-phase action delivery remain TBD with that integration.

### E04 - Component lifecycle

**Status: Agreed**

1. Construct actors/components and apply configuration.
2. ResolveReferences validates dependencies after all objects exist; structural and engine-property mutations are rejected during this pass.
3. Invoke `BeginPlay` for gameplay initialization.
4. Run normal update callbacks.
5. Invoke `EndPlay` when leaving the scene or destroying the object.

- Constructors store configuration; setup requiring the owner, services or other components happens after attachment and reference resolution.
- Loaded objects and runtime-spawned objects follow the same lifecycle principles.
- A resolved dependency need not have completed BeginPlay. The complete frozen batch validates before any start; initially inactive/disabled components still begin once. Only completed BeginPlay calls receive EndPlay. Destructors handle never-started cleanup through RAII.

### E05 - Safe spawning, destruction and references

**Status: Agreed**

- Gameplay may request spawning and destruction during callbacks.
- Newly spawned actors can be created/configured immediately, but enter the active lifecycle and tick set at a defined safe engine boundary.
- Destruction marks an actor for removal immediately, preventing subsequent gameplay ticks. Cleanup and memory release occur at a safe boundary; an already executing callback is not interrupted.
- Stored actor/component references use checked handles so a destroyed target resolves as unavailable rather than leaving a dangling pointer.
- Game code does not manage deletion queues or synchronization.
- ActorRef and ComponentRef<T> resolve checked non-owning identities. Add/Get sees pending objects immediately; one start-of-frame batch is frozen before all catch-up ticks. Destroy invalidates refs immediately and cleanup runs at the next boundary.

### E06 - Scene replacement and persistent session state

**Status: Agreed**

- Start with one active gameplay scene at a time.
- Loading another scene replaces the current one, with a loading screen during the transition.
- Session services and game-session data survive scene replacement; scene actors do not.
- Scene actors receive `EndPlay` during teardown. The new scene constructs its own actors from authored content and relevant session data.
- Persistent player data is separate from the player actor representing it in a scene.
- Seamless streaming and additive gameplay scenes are deferred.
- The loading screen does not imply an agreed background-loading implementation.

### E07 - Import data separately from runtime construction

**Status: Agreed boundary; data contract WIP**

```text
JSON -> SceneImporter::ImportScene() -> ImportedSceneData
     -> SceneBuilder::BuildScene() -> World
```

- The importing work focuses on converting JSON into data structures.
- The engine-side scene builder and component factories are supplied separately.
- Imported data describes actors, transforms, component types and properties. Importing does not create runtime actors or manage engine threads.
- The builder creates runtime objects, applies configuration and resolves references before activation.
- Exact property types, actor/reference encoding and import-result diagnostics must be agreed with the importing team.
- The exporter guide identifies `Archetype` as the source Unreal actor class name. Its mapping to our game construction rules and any separate prefab/template behavior remain TBD.

### E08 - Strict scene validation

**Status: Agreed**

- Structural errors reject the scene before activation; do not silently skip invalid actors/components and start a partial world.
- Structural errors include unknown component types, duplicate actor IDs, invalid required properties and unresolved required references.
- Collect as many useful errors as practical, identifying the actor, component and property involved.
- Optional properties may use explicitly defined defaults. Unknown property names should be rejected to expose authoring mistakes.
- Failed builds clean up temporary objects without invoking `BeginPlay`.
- Replacement-load failure preserves the current scene and asserts in development builds; see E15.

### E09 - Extensible component registration

**Status: Agreed**

- Use a registry of supported component types rather than a central switch over all types.
- Engine code registers built-in components; game code registers game-specific components through the same mechanism.
- Each registration provides a stable type name, creation behavior and explicit property-loading rules.
- Lambdas are suitable registration callbacks. Standard creation/attachment can use a templated helper; property configuration remains explicit per type.
- Define required/optional properties, defaults and validation locally, using shared property-reading helpers.
- Keep construction/attachment separate from applying authored settings and resolving references.
- Adding a supported type does not require changing the JSON importer or generic scene builder.
- Duplicate registered type names are errors; registration completes before scene loading.
- A full reflection/code-generation system is not required initially.

### E10 - Actor hierarchy

**Status: Agreed**

- Support parent-child actor relationships and inherited transforms.
- Destroying a parent also destroys its children.
- Transform parenting is separate from ordinary cross-actor references.
- Activation inheritance follows E16; local/world transform support follows E17. Reparenting behavior and the imported parent representation remain TBD.

### E11 - Requests and notifications

**Status: Agreed**

- Use direct calls for requests to known components/services.
- Use typed events for notifications that may interest several consumers.
- Tie subscriptions to listener lifetime so listeners disconnect when their component or scene is removed.
- Deliver gameplay notifications at a defined update boundary rather than unexpectedly invoking listeners midway through another operation.
- Event delivery phase, ordering, recursive publication and treatment of events during teardown/pause remain TBD.

### E12 - Pause and time

**Status: Agreed**

- Provide scaled game time and unscaled time.
- Ordinary gameplay uses game time and stops receiving update callbacks while paused, rather than repeatedly receiving zero delta time.
- Components that must keep updating during pause opt in explicitly.
- Input, menus and loading can continue independently of paused gameplay.
- Resuming does not catch up on time spent paused.
- Exact time APIs, paused fixed-phase behavior and event/input handling across pause boundaries remain TBD.

### E13 - Explicit component dependencies

**Status: Agreed**

- Components can find siblings through typed lookup, such as `GetComponent<SkeletalMeshComponent>()`; exact API signatures remain to be defined.
- Required dependencies are checked during connection/validation, before any `BeginPlay` calls.
- A missing required dependency rejects scene activation with a diagnostic identifying the actor, dependent component and missing requirement.
- Optional dependencies may be absent and must be handled explicitly by the consuming component.
- The engine does not automatically add missing components; authored composition and configuration remain explicit.
- Dependency declaration syntax and validation of runtime composition changes remain TBD. Required dependency ambiguity follows E14.

### E14 - Multiple components and explicit dependency selection

**Status: Agreed**

- Actors may contain multiple components of the same type.
- A typed collection lookup, such as `GetComponents<T>()`, returns all matching components.
- A component-name lookup can select a specific instance on an actor. Component names must be unique within that actor.
- A required dependency identified only by type must resolve to exactly one matching component: zero matches is a missing dependency; multiple matches is an ambiguous dependency.
- Missing or ambiguous required dependencies reject scene activation during validation. Do not silently choose the first attached component.
- Resolve ambiguity by specifying the intended component name in code or scene configuration, and validate that the selected component matches the required type.
- GetComponent<T> returns the first live match in attachment order, including pending additions; GetComponents<T> returns all. Required type-only resolution rejects ambiguity. Display names may repeat; FindActor returns null with a diagnostic on ambiguity and FindActors returns all. Authored IDs are distinct from display names.

### E15 - Scene replacement failure and assertions

**Status: Agreed**

- Prepare and validate a replacement scene before tearing down the current scene. The candidate is not yet active and does not receive gameplay ticks or `BeginPlay`.
- On successful preparation, end the old scene, activate the new scene and release the old scene's resources. Temporary overlap in scene memory is accepted initially.
- On import/build/validation failure, report detailed diagnostics, discard temporary candidate objects and retain the current scene. Do not activate a partial scene.
- Treat a failed scene load as a development error: assert after diagnostics and cleanup. This is not merely a warning with silent fallback.
- Validation and cleanup must execute independently of assertions. With assertions disabled, return/report the failure and keep the previous scene available.
- An initial scene-load failure also asserts in development builds. With assertions disabled, report the startup failure without entering gameplay; there is no previous scene to restore.
- This preservation guarantee covers preparation failures before committing the transition, not arbitrary failures inside lifecycle callbacks after the old scene has ended.
- Exact error presentation and handling of lifecycle callback failures remain TBD.

### E16 - Inherited actor activation

**Status: Agreed**

- Each actor retains its own local active state, separate from its effective active state in the hierarchy.
- An actor is effectively active only when its local state and the states of all its ancestors are active.
- Deactivating a parent stops gameplay ticks for its descendants without overwriting their local active states or their components' enabled states.
- Reactivating a parent restores ticking only for descendants that are otherwise active; individually disabled children remain disabled.
- IsLocallyActive and IsActiveInHierarchy distinguish local/effective actor activity. There are no activity-change callbacks. Spatial component enablement does not propagate to children. Physics and audio remain deferred.

### E17 - Local and world transform support

**Status: Agreed capability; import conventions WIP**

- The engine must support both local and world transforms.
- Do not assume the external export stores local transforms everywhere. The importer/builder contract must identify each transform's space and conversion rules.
- The noncopyable framework Transform facade hides backend parent pointers; copy parentless LocalPose values explicitly. Actors own permanent root transforms. SetParent applies immediately and reports its actual result.
- Row-vector composition is world = local * parentWorld. KeepLocal/KeepWorld are explicit; singular inversions and unrepresentable local shear reject without modifying pose or parent. Negative/nonuniform/zero local scale is allowed. Export axis/unit conventions remain unverified team inputs.

### E18 - Spatial components and local transform inheritance

**Status: Implemented locally; external adapter verification pending**

- Support transform-bearing spatial components, represented by a SceneComponent layer over the ordinary Component base. Non-spatial behavior components do not need an independent transform.
- Spatial components store local transforms and inherit their parent's transform, with access to the resulting world transform.
- Mesh, light and camera components use their own resolved world transforms rather than only their actor's transform.
- Parentless spatial components inherit their actor root. Component parents must have the same owner; actor parents must belong to the same world. Admitted children cannot attach below a pending parent.
- Imported actor-relative matrices must be converted to runtime parent-local transforms at the import/build boundary; do not assume exported component Transform already has runtime local semantics.
- Destroying an actor or spatial component destroys its attachment subtree; detach first to retain children. Teardown is child-first and reverse start order among peers. Component parent enablement does not control child enablement.

## Team-owned work and open decisions

### T02 - Unreal export adapter and component transforms

**Status: WIP - coordinate with the importing team**

- The supplied importer model targets `UnrealSceneData` with typed mesh/light/material descriptions. Keep exporter-specific data interpretation at the import/build boundary rather than assuming it defines the runtime API.
- Inspected `TestExportMap_Level.json`: 16 actors, each with one component; all component `Parent` fields are empty. Actor entries do not provide an explicit ID or parent field.
- In all 16 samples, component `Transform` multiplied by actor `Transform` matches the component's explicit `TransformBreakdown.WorldTransform` within numerical tolerance. Nested attachments are not demonstrated by this sample.
- Source: supplied `Usage for Programmers _ TGA Wiki.pdf`, revision 12. These are exporter facts, not approval to implement every exported feature.
- The guide specifies matrices as 16 floats ordered by rows, in Unreal coordinates. Component transforms are relative to the owning actor; the root component has identity transform. `Parent` names a parent component, with an empty value for the root. Do not interpret an exported component matrix as parent-component-local without conversion.
- If runtime attachments store parent-local matrices, the builder must derive them from the exported actor-relative matrices after coordinate conversion. Confirm multiplication conventions and validate non-invertible parents; do not apply an ancestor's transform twice. Use a nested attachment fixture before finalizing the adapter.
- The guide names a Space Conversion section, but it is absent from the supplied PDF. Axis conversion and the light-intensity conversion ratio are still unresolved. Light attenuation radius is documented in centimeters and spotlight cone angles in degrees.
- Actor names are documented as unique within the level; component names are unique within their actor. These can identify imported objects within this export, but are not guaranteed stable across renames or a substitute for checked runtime handles.
- `Archetype` is the source actor's C++/Blueprint class name; Blueprint class names end in `_C`. How the game maps those classes to composition/behavior remains TBD; this does not imply executing Blueprint logic.
- Component TypeIDs are documented: 0 scene, 1 static mesh, 2 skeletal mesh, 3 point light, 4 spot light, 5 directional light, 6 box, 7 sphere, 8 capsule, 9 spring arm; -1 denotes custom serialization. Use a signed field or equivalent explicit representation instead of the proposed unsigned field. The custom payload/type identity contract still needs agreement.
- Mesh material arrays preserve slot order. `ContentPath` and texture paths are Unreal asset identifiers, not directly usable local file paths; resolution belongs to the team-owned asset integration.
- The sample contains scalar (Type 0), color-array (Type 1), and texture-object (Type 3) material parameters. A color/texture-only variant cannot represent all supplied data.
- Proposed `vector<ComponentData>` storage would slice derived mesh/light fields. Agree on a value variant or polymorphic owning representation that preserves the concrete payload.
- Material descriptions should be separate from actor component descriptions; material parent references must not be confused with transform parents.
- Agree on the supported subset of documented TypeIDs, fixed-size matrix representation, component identity/reference rules and game mapping of Archetype. Unsupported types must be handled explicitly under strict scene validation rather than silently dropped.
- Runtime render components use their composed component world transform. Export conversion still requires real nested fixtures; historical source assertions below are not reverified against current Perforce code.
- Importer handoff concerns are collected in `ImporterHandoff.md`.

### T01 - Asset access and management

**Status: TBD - coordinate with the team; may already be WIP elsewhere**

- Asset access may be designed by other team members. Confirm ownership and inspect their design before introducing a competing service.
- No choice has been accepted yet between content-relative paths and stable asset IDs.
- `GetAssets()`, cache behavior, asset handles, retention/unloading and synchronous/background loading are proposals only, not approved contracts.
- Agree on the scene-builder integration, missing-asset diagnostics, packaging paths and safe runtime resource changes with the asset-system owners.

### Remaining engine decisions

| Area | Outstanding decision |
| --- | --- |
| Input integration | Inspect the existing action system; define UI capture, binding configuration and action sampling semantics. |
| Component dependencies | Declaration/lookup APIs and validation of runtime composition changes; multiplicity and required dependency selection are agreed in E14. |
| Frame order | Exact lifecycle/event flush points, tick dependencies, physics placement and overload policy. |
| Hierarchy | Spatial attachment/root APIs, import conversion, reparenting, cycle validation and component attachment lifecycle; see E16-E18 and T02. |
| Scene contract | Property value types, references, schema versioning, archetypes and schema-change ownership. |
| Scene failure | Error presentation and lifecycle callback failures; preparation failure handling and development assertions are agreed in E15. |
| Audio | Service integration, ownership of playing sounds and pause behavior. |
| UI | Integration, focus/cursor ownership and its relationship to gameplay input. |
| Save/load | Persistence boundaries, serialization/versioning and restoration of references. |
| Background work | Completion delivery, cancellation and ownership during scene teardown. |
| Tooling | Actor inspection, scene-error reporting, profiling and build/package workflow. |
| Performance | Reference machine, resolution, representative workload and measurement procedure. |
| Delivery | First playable acceptance criteria, API review ownership and required automated checks. |

Update this register as decisions are accepted. Preserve unresolved items as TBD/WIP
rather than presenting a suggested API or current implementation shortcut as a
settled architectural contract.
