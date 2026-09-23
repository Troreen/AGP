# Changes since the first Perforce integration

This document is a simple overview of the changes made after the first
GameFramework/Unreal scene integration.

## Input system

Orhan's `CommonUtilities::InputMapper` is the sole mapping system, restored from commit `6d730498ba9b41e3f7ef3bf6dc173cebb6d510c7`.

- Win32 messages reach InputHandler through the window procedure. InputMapper advances the handlers once per frame and dispatches listeners directly.
- Game, runtime and components bind their own named actions. Callbacks inspect `InputEvent.inputData.isPressed`, `isHeld`, `isReleased`, and axis values.
- Components remove their unsigned listener IDs in EndPlay; Game does so in Shutdown. Listener mutation and scene changes happen outside mapper dispatch.
- ServiceLocator owns InputMapper and borrows AudioManager and AssetRegistry. The runtime owns the mapper's device handlers.
- Only camera movement/mouse look and F1/F4/F5/F6/F7/F8 bindings remain. Animation, lights, tonemapping, spin toggles, diagnostics printing and Escape input bindings have been removed.
- Focus loss releases held keys on the next mapper update. No default gamepad bindings are installed.

Time-based movement still happens in component Update. See [restoration details and limitations](../InputRestorationPlan.md).

## Debug camera

- F1 toggles a debug camera.
- The debug camera is created only when it is needed.
- Toggling back restores the previous camera if it still exists.
- If a scene has no selected camera, the debug camera is used automatically.
- Saved camera state is cleared when the scene changes.
- The debug camera's transform, projection, movement speed and controls are kept
  together in its configuration.

## Transform system

- `CommonUtilities::Transform` was removed.
- GameFramework now owns the only public `Transform` class.
- `LocalPose` was replaced by the copyable `TransformData` structure.
- Public rotation uses Euler degrees: yaw around Y, pitch around X and roll around Z.
- Quaternions are only used internally for math and rendering conversion.
- Runtime `Transform` objects cannot be copied or moved.
- Transform setters reject invalid numbers without partially changing the transform.
- Actors are transform roots. Spatial component transforms are relative to their Actor.
- A component world matrix is `componentLocal * actorWorld`.
- Public transform parenting and the unused parent pointer were removed.

## Unreal importer and adapter

The scene flow is now:

`JSON -> UnrealSceneData -> UnrealSceneAdapter -> SceneData -> candidate World`

The importer stays close to the team's switch-based implementation. It reads the
exported data but does not decide how the engine should interpret coordinates.

- Exported matrices are kept unchanged by the importer.
- Axis mapping, optional unit scaling and matrix decomposition happen in
  `UnrealSceneAdapter`.
- `TransformBreakdown` is intentionally not parsed or validated.
- The importer supports the same material parameter types as the team version:
  scalar, Vector3f and texture. Other material parameter types are ignored for now.
- Missing or malformed files return a diagnostic instead of looking like a valid
  empty scene.
- Unknown component `TypeID` values reject the import instead of being silently skipped.
- Scene components are retained. The adapter decides whether they become ordinary
  scene components or tagged cameras.
- Static and skeletal mesh identities remain distinct.
- Mesh name and exported content path are both retained.
- Box, sphere, capsule and spring-arm records keep their common component data and
  their exported shape settings.

The following bugs from the first integration were fixed and are commented at the
changed code:

- Materials and material parameters were inserted twice.
- Every light color channel was read from the red channel.
- Point lights were marked as spot lights.
- Several placeholder component types were marked as directional lights.
- Capsule records lost their name, tags, parent and transform.
- Spring-arm settings were not read.
- Skeletal meshes were changed into static meshes.
- Unknown component types were silently discarded.

`UnrealSceneImporter copy.cpp` is only a historical comparison copy. It is not
compiled by the GameFramework project.

## Runtime scene data

- The old generic property map and string component factories were replaced with
  strongly typed scene records.
- Runtime records exist for scene components, cameras, static meshes, skeletal
  meshes, directional lights, point lights and spot lights.
- Box, sphere, capsule, spring-arm and custom components currently use typed
  placeholder records. Their imported data is retained even though they have no
  behavior yet.
- Actor name, archetype, tags and transform are stored on the Actor.
- Component name, tags, source parent and transform are stored on the Component.
- Mesh identity and content path are stored on mesh components.
- Light settings are stored on light components.
- Imported parent names are informational only. Runtime transform parenting was
  not added.

## Assets and materials

- `IAssetResolver` is the small boundary used to resolve meshes, textures and
  parent materials.
- The current `AssetLibrary` implements that boundary until a separate asset
  manager is available.
- Each imported material slot creates its own material instance.
- Material slot order is preserved.
- Scalar, vector and texture parameters are applied without modifying a shared
  parent material.

## Scene construction and replacement

- `ComponentRegistry` builds a new candidate World from typed `SceneData`.
- The game receives `ConfigureWorld` after built-in objects exist and before
  `BeginPlay`. This is where game-specific behavior is attached from tags or archetypes.
- Scene and asset construction errors reject the candidate World.
- A failed scene load leaves the current World running.
- A successful load replaces the World only after construction succeeds.
- The `ActiveCamera` tag selects an imported camera. Without one, the debug-camera
  fallback is activated.

## Removed legacy paths

- ServiceLocator is restored as the central engine-service access point.
- Polling input accessors were removed.
- The old `ModelViewer` path was removed after its controls were migrated.
- The duplicated free-fly camera controllers were replaced by the GameFramework
  debug-camera controller.
- The old generic scene reader and built-in string factories were removed.

## Verification

Tests now cover transform validation, mapper events and explicit listener cleanup, camera
controls, scene replacement, importer failures, typed scene conversion, material
ordering, metadata ownership and debug-camera behavior. Public-header tests also
check that gameplay-facing transform APIs do not expose quaternion types.
