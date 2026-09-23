# Restore Orhan's InputMapper and ServiceLocator

Status: implementation plan only. No engine source changed.

## Objective and source of truth

Restore from local Git commit `6d730498ba9b41e3f7ef3bf6dc173cebb6d510c7`:

- `Source/Utilities/CommonUtilities/InputMapper.h` and `.cpp`.
- `Source/Engine/GameFramework/ServiceLocator.h` and `.cpp`.

The supplied recovery investigation identifies these as Perforce InputMapper #2 / CL 491430 and ServiceLocator #1 / CL 491357. This planning pass inspected the Git originals; it did not independently query Perforce. Git reports no changes to the supporting handlers and key/gamepad/pointer enums between that commit and current HEAD.

Target: zero changes to Orhan's InputMapper files. Restore ServiceLocator's original singleton restrictions, owned mapper pointer, getter/setter, destructor, and KillServices behavior. Permit only narrowly justified additions for today's audio and asset services.

Do not restore historical ModelViewer or FreeFlyCameraController wholesale. Use them as integration references while retaining today's world, components, scenes, rendering, camera behavior, audio, and assets.

## Recommended architecture

Keep the current gameplay-facing InputSystem API as a compatibility layer, backed by Orhan's real mapper. This avoids rewriting every component and weakening existing callback lifetime guarantees. The mapper must actually bind inputs and emit the live events; merely restoring unused files or calling Update before the existing raw-device capture is insufficient.

Runtime flow:

`Win32 messages -> InputHandler -> InputMapper::Update -> copied events -> compatibility action processing -> gameplay callbacks -> game/world update`

Add a small runtime bridge outside the four restored files. The host owns this bridge and the existing InputHandler/XInputHandler. ServiceLocator owns the heap-allocated mapper. GameContext continues to own the gameplay compatibility layer; World and components keep their existing accessors and subscriptions.

The compatibility layer has an explicit purpose: modifier precedence, multiple bindings per action, focus handling, current Started/Ongoing/Ended behavior, stable dispatch ordering, and RAII subscriptions. These are current engine requirements absent from the historical mapper. Keep them out of CommonUtilities and out of ServiceLocator.

## 1. Establish a baseline

1. Record HEAD, working-tree status, and hashes of the four historical files. Preserve the existing untracked `Runtime/RuntimeRefactoringPlan.md`; this restoration is a separate change.
2. Build the current engine and test projects before implementation. Record pre-existing failures separately. For example, GameRuntimeTests currently refers to GameContext::GetSceneName(), whereas the inspected header exposes GetSceneType(); establish actual build status before attributing failures to restoration.
3. Record current input behavior using existing tests and focused event traces: press/hold/release, modifiers changing while the main key stays held, simultaneous bindings, mouse look, focus loss, scene reload, and subscription removal.
4. Treat this restoration as superseding the other plan's proposal to merely refactor the current input implementation. Do not perform both input changes concurrently.

## 2. Restore the originals and reconcile services

Restore the four files from the exact commit. Add InputMapper to CommonUtilities.vcxproj and its filters without reverting other project entries.

Keep ServiceLocator's original ownership contract:

- SetInputMapper receives ownership of a heap object. Never pass a stack object or a pointer still owned by a unique_ptr.
- Initialize a mapper under temporary RAII ownership; transfer it once startup is ready. Verify the locator is empty before transfer, since the original setter does not delete an existing mapper.
- KillServices deletes and nulls the mapper. Leave that operation intact and safe to call again.
- Keep the mapper's handler pointers borrowed. The handlers outlive all mapper updates and mapper destruction.

Add only the existing non-owning audio/asset registration and checked getters, initialized to null. Extend KillServices to null those borrowed pointers without deleting their objects. AudioManager::Shutdown and AssetRegistry::Clear remain explicit host responsibilities.

Remove host usage of ProvideInput/GetInputSystem/Clear on the locator; use SetInputMapper/GetInputMapper/KillServices. The current global GetInputSystem usage found outside startup is a test assertion; change that assertion to verify the restored mapper and its bridge connection. GameContext/World/Component::GetInputSystem remain valid compatibility accessors.

Strong justification for the locator additions: Game.cpp and the host already use the audio service, and runtime tests exercise asset registration. Preserving these access paths and owners avoids unrelated lifecycle changes. Do not add an owned audio/asset pointer or a generic service registry.

## 3. Connect bindings and callbacks outside the mapper

Implement bridge registration using BindActionToInputCode and AddEventListener. Install registrations before the first update and modify/remove them only outside mapper dispatch.

The original mapper stores one code per action per device category. Therefore use stable internal names for distinct physical sources, then map those sources to public gameplay actions. Register each physical gamepad stick/trigger once and fan out afterward: the original mapper stores transition flags per physical axis, not per binding.

Mapper callbacks only copy input fields into bridge storage. InputEvent contains a reference to temporary InputData: never queue the original event or its reference. After Update returns, evaluate public actions and dispatch gameplay callbacks through the existing subscription machinery. This also prevents gameplay operations such as debug-camera creation from mutating the mapper's listener container mid-iteration.

Preserve these compatibility rules explicitly:

- Mouse-button events are button values for current gameplay even though Orhan includes cursor coordinates and sets isAxis2D too.
- Button sources retain held state; pointer delta is cleared each frame because the mapper emits no stationary-mouse event.
- Required/forbidden modifiers and the existing specificity rule are evaluated after all sources are collected. Modifier changes must affect an already-held main key.
- Multiple bindings remain active until all applicable sources release. Preserve current ordering and value precedence, including Ended carrying the previous value.
- Focus loss terminates active gameplay actions and clears mouse anchoring. Device state collection can continue while gameplay delivery is suppressed.
- Gamepad disconnect must clear active gamepad contributions even though the mapper returns without release events. Derive this from that frame's absence of gamepad activity; do not add an extra XInput connectivity poll. Test reconnect as well.
- ClearBindings and scene Reset clear their intended compatibility state without destroying unrelated mapper registrations or listeners.

Reuse the existing action-processing logic where possible. InputDeviceFrame may remain a deterministic test input to that logic, but production must obtain its mapped source values through mapper events. Keep synthetic injection and live collection mutually exclusive per frame. Add bridge coverage so passing synthetic-only tests cannot conceal a disconnected mapper.

## 4. Integrate the frame loop and mouse handling

In GameApplication.cpp, keep forwarding messages through InputHandler::UpdateEvents. Replace the standalone InputHandler::UpdateInput and XInputHandler::UpdateInput calls with exactly one InputMapper::Update per processed frame. The historical mapper invokes both handlers itself. XInput's internal reconnect probing remains its own implementation detail.

Sequence: process messages and pending scene changes; prepare focus/mouse state; clear transient bridge values; update mapper; finalize action state and callbacks; update game/world/audio; render.

Replace CaptureInputFrame's full raw-device capture with bridge collection and the necessary host mouse policy. Retain right-button gating, EnableMouseLook, first-frame zero delta, focus/scene anchor resets, direction and sensitivity. Orhan's message-derived MOUSE_DELTA differs from the current cursor-to-center measurement: validate these paths with real cursor warping before choosing the host implementation. Prefer mapped delta plus host centering; if behavior requires keeping cursor-to-center normalization, keep that narrowly scoped mouse policy in the host and document the reason. Never combine both deltas or advance handler state a second time.

Test release outside the window, minimize/restore, and focus regain. If existing tentative key state needs clearing on focus loss, do it in the host using the handler's public event API; do not change InputMapper to understand windows or focus.

## 5. Make cleanup and scene transitions explicit

Bindings belong to the session; gameplay subscriptions belong to their host/game/components. Scene reload removes old component subscriptions and resets compatibility action/mouse state, while preserving the session mapper and host bindings. Failed candidate-world construction must leave the live scene and its listeners intact.

Normal and exceptional teardown must share this order:

1. Stop updates and new scene requests.
2. Shut down gameplay and clear the world while services remain available.
3. Release host subscriptions, detach bridge listeners, and clear bridge callbacks.
4. Call KillServices while the borrowed handlers still exist.
5. Shut down audio and clear assets through their existing owners.

Ensure cleanup still runs if game shutdown throws; do not rely on the current normal-path sequence alone. Cover partial initialization and a second application run in the same process. Do not copy the historical camera destructor's global binding removal into today's multi-component lifecycle.

## 6. Verification and completion criteria

Build AGP.sln and Game.sln in supported x64 configurations (Debug, Release, Retail). Run GameFrameworkTests, CameraControlsTests through their owning test target, GameRuntimeTests, PublicGameplayConsumer, public-header isolation, and EngineOptimisationsTests where supported. Report environmental and pre-existing blockers accurately.

Add focused restoration tests:

- Real InputHandler messages through the unchanged mapper: key and mouse-button press/hold/release and copied payload lifetime.
- Bridge-to-gameplay traces compared with baseline: modifiers, multi-binding, phase/value behavior, callback ordering/removal, and subscriptions created by debug-camera activation.
- Mapper update and handler state-advance counts: one per processed frame, with no second capture path.
- Scene success/failure/reload, subscriber destruction, focus/minimize transitions, controller disconnect/reconnect, partial startup failure, throwing shutdown, repeated cleanup and repeated runs.
- Gamepad integration using a controlled test-side XInput boundary or a real controller; synthetic InputSystem frames alone are insufficient to validate the restored mapper.

Manual smoke test: movement and right-button look; F1 debug camera; F4 reload; F5/F6 render passes; F7/F8 demo actions; R spin; numpad animations and tonemapping; plain/Shift lighting shortcuts; P diagnostics/lights; Escape; music continuing and stopping correctly. Check specifically that numpad 7 retains tonemapper selection while regular 7 retains the directional-light shortcut.

Completion requires the mapper to drive live input, original ownership to be restored, current behavior checks to pass, and no unexplained edits to the originals. Compare InputMapper.h/.cpp against the historical blobs with line-ending normalization. Review ServiceLocator differences line by line.

## Change-control rule for Orhan's four files

Every deviation must state: the reproduced failure or concrete compatibility requirement; why host/bridge/caller changes cannot reasonably solve it; the smallest proposed edit; and the validating test. Formatting, naming preference, modernization, convenience, and speculative fixes are insufficient reasons.

Known original constraints are handled externally: direct listener-container iteration, referenced event payloads, one binding per category, per-axis transition flags, and ClearBindingsFromAction returning after removing the first matching category. Use one source/category per internal action so cleanup does not depend on changing that method.

Expected final source delta: InputMapper unchanged; ServiceLocator minimally extended for borrowed audio/assets; integration and compatibility changes concentrated in the runtime plus project entries and tests. Split review into historical restoration, justified locator extensions, runtime integration, and verification. Do not merge a partially wired restoration.
