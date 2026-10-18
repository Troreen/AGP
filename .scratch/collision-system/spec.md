# General collision detection MVP

Status: in-progress

Implementation gate satisfied: the user explicitly authorized source implementation in the agp-feature resume request. Changes remain unstaged and uncommitted.

## Problem Statement

Gameplay needs a general shape-detection foundation that reports overlaps through AGP's existing EventBus. Collision owns geometry, filtering, and overlap phases; consuming gameplay systems decide what those results mean. V1 detects overlaps without physical blocking.

## Solution

Add one ColliderComponent per Actor, supporting a sphere or yaw-rotated box. A World-owned CollisionSystem performs custom overlap tests after gameplay updates, tracks pairs, and publishes Enter/Stay/Exit notifications on the shared EventBus. Gameplay owns volume spawning, positioning, enable/disable, lifetime, and event reactions. No external collision library.

## User Stories

1. As a gameplay programmer, I can spawn an Actor with one named collider volume in code.
2. As a gameplay programmer, I can choose a sphere or box and set its dimensions and Actor-relative transform.
3. As a gameplay programmer, I can subscribe to overlap events, identify the involved volumes, and unsubscribe safely.
4. As a gameplay programmer, I receive distinct Enter, Stay, and Exit phases for each overlapping pair.
5. As a gameplay programmer, I can disable or destroy a volume without leaving stale overlap state.
6. As a gameplay programmer, I can filter overlaps by layer and mask.
7. As a gameplay programmer, I can reuse the same detection and notification behavior across independently positioned volumes without adding gameplay rules to collision.

## Implementation Decisions

### Ownership and scope

- Collision code belongs in GameFramework. Each World owns one concrete CollisionSystem and its pair state; Actors own their ColliderComponents.
- One ColliderComponent per Actor, including disabled Components. Reject a second non-destroyed collider clearly before it participates; do not silently choose the first. Replacing a pending-destroy collider must not reuse its ID.
- One ColliderComponent class derives from SceneComponent and selects Sphere or Box. No shape class hierarchy, singleton, plugin interface, compound colliders, or separate scene graph.
- Default shape: sphere of radius 1. Box dimensions use half-extents. Shape setters select the corresponding shape.
- Use current composed Component/Actor transforms, rather than cached physics poses or Update(0) refresh calls.
- Code spawning only. Existing imported box/sphere/capsule placeholders remain unchanged.

### Shapes and filtering

- Support sphere/sphere, sphere/box, and box/box tests. Boxes rotate around world up only. Test actual oriented boxes; an enclosing AABB is only an optional early rejection.
- Box half-extents and sphere radius must be finite and positive. Invalid setter input fails without changing the previous valid configuration.
- Boxes permit positive nonuniform scale without shear. Spheres require positive uniform scale. Reject unsupported/nonfinite transforms from the current detection pass with a diagnostic; do not silently approximate their shapes. A previously overlapping pair then exits.
- Touching counts as overlap. Use CU math and existing valid intersection helpers; keep additional geometry functions internal and focused.
- Each collider belongs to one numbered layer in the range 0-31 and has a 32-bit detection mask. Gameplay owns layer meanings; the engine provides no named layer registry.
- Default layer is 0; default mask allows all 32 layers. A pair is eligible only when each mask includes the bit for the other collider's layer. A zero mask is valid and detects nothing. All colliders are nonblocking.
- Same-Actor interactions need no machinery because only one collider is allowed per Actor.

### Detection and lifecycle

- Reuse World's frozen frame membership. Components created inside BeginPlay/Update/callbacks wait until the next eligible World update. Components created before the frame is collected may participate after BeginPlay.
- After normal Component updates, gather begun, enabled colliders owned by active Actors from that frame, derive their current world shapes, and test each eligible unordered pair once.
- Initially use a simple pair loop. Do not introduce spatial trees, parallel jobs, performance claims, or benchmarks without a demonstrated need.
- New overlapping pair: Enter. Existing overlapping pair: Stay. Removed pair: Exit. Enter and Stay are mutually exclusive per pair per update.
- Track real ordered ID pairs with equality; never use a mixed hash alone as pair identity.
- Collect the complete event batch before dispatch. Gameplay callbacks cannot change the already collected batch. Their object/configuration changes affect the next detection pass.
- Disable, deactivation, destruction, or filter changes remove participation and cause Exit on the next pass if the pair existed. Re-enabling later can cause a fresh Enter.
- Cleanup may release Components before Exit dispatch. Pair state and event payloads therefore retain IDs, never dereference dead Components.
- World::Clear discards pair state without publishing teardown events. Keep existing World lifecycle recursion/exception guards.
- Detection is frame-sampled. A volume crossing another entirely between samples may be missed; continuous detection is not part of v1.

### Public interface contract

- ColliderComponent exposes SetSphereRadius, SetBoxHalfExtents, SetLayer/GetLayer, SetMask/GetMask, GetColliderId, and inherited transform/enable/destruction APIs. SetLayer accepts an index in 0-31; SetMask accepts a 32-bit unsigned bitmask.
- Shape/layer/mask setters return a boolean result, preserving prior valid values on invalid input. Out-of-range layers fail; every 32-bit mask is valid.
- Collider IDs are nonzero world-local monotonically assigned identities, not reused during the World object's lifetime, including Clear. Assign when attached so configuration/subscription code can read an identity before BeginPlay.
- World exposes FindCollider(id), returning a borrowed ColliderComponent pointer or null for a missing/pending-destroy Component. Disabled Components remain findable while alive.
- One CollisionOverlapEvent type contains SourceWorld, FirstColliderId, SecondColliderId, and Phase (Enter/Stay/Exit). Order the IDs consistently. Publish one event per pair, not one event per participant.
- SourceWorld is borrowed and valid for synchronous dispatch. IDs are interpreted in that World. Do not retain World/Component pointers across destruction or scene replacement. Exit lookup may return null.
- Subscribers inspect IDs/layer/owner as appropriate and own gameplay effects. The system publishes overlap phases without prescribing a gameplay reaction or adding listener-per-volume convenience infrastructure.

### Existing EventBus

- Keep EventBus.hpp and EventBus.cpp unchanged: this service is owned by another team member.
- Reuse ServiceLocator's EventBus, AddEventListener, RemoveEventListener, and Dispatch as they already exist. The collision system only publishes CollisionOverlapEvent values.
- Gameplay Components subscribe in BeginPlay, store the returned token as std::uint32_t in their own code, and unsubscribe in EndPlay. Do not require the private EventBus::ListenerID alias to be public.
- EventBus implementation changes and new subscription-mutation guarantees are outside this feature. The earlier enhancement contract and tests are superseded by the user's ownership decision.

## Testing Decisions

Use existing GameFramework test conventions. Observe behavior through World, ColliderComponent, EventBus, and small test gameplay Components; do not test private pair containers or duplicate implementation formulas.

| Criterion | Observable acceptance | Ticket |
| --- | --- | --- |
| T1 | Collision integration uses the unchanged EventBus API; gameplay stores the existing subscription token and subscribes/unsubscribes through normal Component lifecycle. | 01, 02 |
| T2 | A code-spawned sphere pair produces Enter, then Stay, then Exit; one collider per Actor is enforced. | 02 |
| T3 | Layers/masks, transforms, multiple occupants, zero-delta updates, and frame membership behave as specified. | 02 |
| T4 | Disable, deactivation, destroy, replacement, callback mutation, and Clear leave no stale/dangling state; Exit IDs and lookup work. | 02 |
| T5 | Sphere/box and box/box overlaps honor yaw, offset, scale, touching, and different heights; invalid geometry is explicit. | 03 |
| T6 | An opt-in runtime demonstration spawns visible debug volumes and a subscribing gameplay Component, and shows phases, enable/disable, destruction, and unsubscription. | 04 |
| T7 | Debug/Release x64 solution builds, affected framework regressions, public-header isolation, and separate spec/standards review pass with evidence. | 04, with affected checks per earlier ticket |

Suggested scenarios: default layer/mask behavior; layers 0 and 31; one-sided rejection; zero mask; out-of-range layer rejection; filter changes causing Exit/re-entry; all shape pairings including reverse order; rotated-box AABB false positives; tangency; separation on Y; independent simultaneous occupants; one callback destroys a participant; unsubscribe during delivery; re-enable after Exit; scene replacement with no old listeners.

The runtime demonstration is opt-in from code using one named debug boolean, disabled by default. Reuse Game::ConfigureWorld and LineDrawer; no new settings format or permanent controls. Spawn separated Actors for each volume, move one slowly through the others, log phases, and visibly exercise enable/disable and destruction. Record exact run steps and evidence when implemented; a successful build is not runtime evidence.

## Out of Scope

Multiple collider Components on one Actor; capsules; blocking/sliding/penetration correction; placement recovery; navigation; projectiles/sweeps; explicit overlap queries or querying disabled shapes; Unreal importer/tag/exporter changes; combat behavior and gameplay-specific rules; physics libraries; spatial acceleration; debug/editor frameworks; automatic collider lifetimes.

## Further Notes

### Intended API example

Illustrative gameplay usage, to be implemented by the tickets (not existing APIs yet). The owning gameplay Component retains the listener ID and removes it during EndPlay. This example is included to satisfy the user's explicit request for public API examples.

```cpp
Actor* volumeActor = world.SpawnActor("DetectionVolume");
auto* collider = volumeActor->AddComponent<ColliderComponent>("Collider");
collider->SetSphereRadius(100.0f);
collider->GetTransform().SetLocalPosition({0.0f, 90.0f, 0.0f});

const auto colliderId = collider->GetColliderId();
EventBus& bus = ServiceLocator::GetInstance().GetEventBus();
const auto listenerId = bus.AddEventListener<CollisionOverlapEvent>(
    [colliderId, &world](const CollisionOverlapEvent& event)
    {
        if (event.SourceWorld != &world ||
            (event.FirstColliderId != colliderId && event.SecondColliderId != colliderId))
        {
            return;
        }
        // React to event.Phase here using the subscribing Component's gameplay behavior.
    });

// Later, in the subscribing gameplay Component's EndPlay:
bus.RemoveEventListener(listenerId);
```

The position in this example is relative to the newly spawned volume Actor. Each independently positioned volume uses a separate Actor with its own ColliderComponent. Gameplay controls any movement or following behavior.

The former broad collision proposal and combat-driven motivation are superseded. Reference investigation remains historical context in research.md. AGP v1 keeps one overlap implementation shared internally by automatic detection; gameplay use cases do not define its categories or geometry.

Keep the existing feature directory for continuity. See workflow.md and the numbered issues. No source changes, staging, commits, or implementation are authorized by the publication of these planning records alone.
