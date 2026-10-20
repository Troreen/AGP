# Collision detection

Collision detects nonblocking overlaps between spheres and yaw-rotated boxes.
Gameplay decides what Enter, Stay, and Exit mean. There is no movement response,
combat rule or continuous/swept detection in this version. The Unreal scene importer
supports exported box and sphere components.

## Spawn and configure

Include `GameFramework/Components/ColliderComponent.h` and
`GameFramework/World/World.h`. The collider header includes the shared
`GameFramework/Collision/CollisionLayers.h`. Each independent volume needs its own Actor:

```cpp
Actor* actor = world.SpawnActor("DetectionVolume");
auto* collider = actor->AddComponent<ColliderComponent>("Collider");
collider->SetSphereRadius(100.0f); // Or SetBoxHalfExtents({100, 50, 80}).
actor->GetTransform().SetWorldPosition({0, 100, 0});
collider->GetTransform().SetLocalPosition({0, 20, 0}); // Actor-relative offset.
collider->SetLayer(CollisionLayer::Trigger);
collider->SetOverlapsWith({CollisionLayer::Player, CollisionLayer::Enemy});
const ColliderId id = collider->GetColliderId(); // Available immediately after attachment.
```

One non-destroyed collider is allowed per Actor, even if disabled. Adding another
throws `std::invalid_argument`. Layer setters return false for invalid input and
preserve their previous configuration. Shape setters directly assign dimensions
and select the shape; supply a positive sphere radius or positive box half-extents.
Author boxes with positive scale and world-up yaw,
and spheres with positive uniform scale. Transforms must contain valid numbers
and no skew. Collision uses the supplied transforms directly and does not validate
these authoring rules at runtime.

`GameFramework/Collision/CollisionLayers.h` defines the shared `CollisionLayer`
enum: Default, Player, Trigger, and Enemy. Edit this header to choose layers for
each game. Values must stay within 0 through 31; filtering uses a bitmask internally.
Both colliders must accept each other's layer. All combinations remain nonblocking.

`SetOverlapsWith` replaces the accepted layers and returns false if any layer is
outside 0 through 31, preserving the previous filter. Repeating a layer is harmless.
An empty list accepts nothing. `SetOverlapsWithAll()` accepts every layer;
`SetOverlapsWithNone()` accepts none. New colliders start on Default and accept all.
Gameplay does not need bit arithmetic. Raw `SetMask`/`GetMask` remain available.
`SetMask` directly assigns the mask and returns no value.

`SetLayer`, `GetLayer`, and `SetOverlapsWith` all use the same
`CollisionLayer` enum. There are no numeric or templated layer overloads.

## Import shapes from Unreal

Use a Box Collision or Sphere Collision component. The existing exporter provides
box `Bounds` (half-extents), sphere `Radius`, component `Transform`, and `Tags`.
AGP creates a `ColliderComponent` from that data without changing the exporter.
Dimensions remain local; the Actor and component transforms apply scale once.
The shape can be the Actor root or attach directly to its root. Deeper attachment
chains produce import errors. Multiple supported shapes on one Actor are rejected
when the builder attaches them, using the same one-collider rule as code-created
shapes. Dimensions are used as authored. Capsule components remain placeholders.

Imported shapes start enabled on layer 0 with all mask bits set. Tags are
preserved as ordinary metadata; the importer does not interpret collision tags.
Gameplay code chooses layers with `SetLayer` and `SetOverlapsWith`, either when creating
an Actor or in a component's `BeginPlay`. `ScriptedEventComponent` configures
its named trigger collider to use Trigger and accept Player in `BeginPlay`.

The exporter does not supply Unreal Collision Enabled, Generate Overlap Events,
Object Type, or response tables. AGP does not read those settings or infer blocking
behavior from them. Additional authoring settings are deferred.

## Debug outlines

Each collider owns its drawing settings. Drawing defaults off and is independent
of collision enablement:

```cpp
collider->SetDebugDrawEnabled(true);
collider->SetDebugColor({0, 1, 1, 1}); // RGBA; default green.
collider->SetDebugDisabledColor({0.4f, 0.4f, 0.4f, 1}); // Default gray.
// Later: collider->SetDebugDrawEnabled(false);
```

World automatically queues sphere rings or box edges after normal updates and
collision callbacks, using the collider's current composed transform. Disabled
colliders use the disabled color; inactive Actors and destroyed colliders do not
draw. Colors and the toggle have matching getters. Outlines show authored geometry.
The renderer clears these temporary lines each rendered frame.

## Receive collisions in a Component

Put a Component on an Actor with a collider and override the phases it needs.
Include `GameFramework/World/Component.h` for the base class and
`GameFramework/Collision/CollisionOverlapEvent.h` when accessing collision details.
No subscription, listener ID, or cleanup is needed:

```cpp
class DetectionListener : public Component
{
public:
    void OnCollisionEnter(const Collision& collision) override
    {
        if (Actor* other = collision.OtherActor())
        {
            // Apply your gameplay rules to the entering Actor.
        }
    }
};
```

`OnCollisionStay` and `OnCollisionExit` use the same signature. The default methods
on Component do nothing. Every enabled Component that has begun play on each
colliding Actor receives the matching method, in attachment order. Inactive or
destroyed Actors and disabled or destroyed Components receive nothing. An Actor
without a collider does not receive collision methods.

`collision.OtherCollider()` and `collision.OtherActor()` look up the other participant;
either can return null after destruction. `OtherColliderId` remains available even
when lookup returns null. `Phase` identifies Enter, Stay, or Exit. The collision
and returned pointers are borrowed for the method call only.

Delivery uses the World's frozen frame membership. Adding a Component inside a
handler cannot give it collisions before its next frame's BeginPlay. Disabling
or destroying a Component skips any pending calls to it in the same batch.
Destroying the receiving collider also stops delivery to its Actor. A disabled
collider can still produce Exit for the Actor's other enabled Components.

The gameplay example is `ScriptedEventComponent`: its optional `TriggerCollider`
names a collider on the same Actor. BeginPlay configures its filtering, and
`OnCollisionEnter` starts the sequence and disables that collider on success.
An empty or invalid TriggerCollider does not activate collision playback.

## Timing and lifetime

Detection follows normal Component updates using the World's frozen frame
membership. Components added during callbacks or updates wait for the next
eligible frame. Touching counts as collision. One unordered pair produces Enter
on its first sampled collision, Stay on later overlapping updates, and Exit after
separation or loss of participation. Enter and Stay never both occur for the
same pair in one update.

Disable, Actor deactivation, destruction, and mask changes cause Exit on the next
detection pass. Re-enabling can produce a new Enter. World clearing discards pair
state without teardown events. The complete event batch is collected before
dispatch; callback changes affect the next pass, including when an earlier
callback destroys a participant mentioned by a later event.

`world.FindCollider(id)` returns a borrowed pointer or null for a missing or
pending-destroy collider. Disabled colliders remain findable. Exit lookup may be
null. Never retain lookup pointers across destruction or scene replacement.
`SourceWorld` is borrowed for synchronous delivery. IDs belong to that World,
are nonzero, and are not reused during its lifetime, including after Clear.

The existing `CollisionOverlapEvent` EventBus API remains available for diagnostics
and listeners that need every pair. Component collision methods run before that pair's
EventBus event. Gameplay normally overrides the Component methods above, which
receive only their own Actor's collisions.

Detection samples frames. A fast volume may cross another entirely between
updates without an event. Direct queries, capsules, compound colliders, blocking,
and physical resolution are outside this version.

## Verify in a scene

Debug Game builds enable outlines for all scene colliders. The optional code-spawned
demonstration has been removed. Imported boxes and spheres now become runtime
colliders. `Game.cpp` creates Player with a radius-60 sphere offset 90 units up,
on the Player layer with a mask accepting Trigger. This is a body approximation
that needs visual confirmation against the character.

See [the Unreal collision handoff](ScriptedEventCollisionTriggers.md) for the
authoring and round-trip checks. Verify placement, entry, activation, disabled gray
outlines, and F4 reload in Game. The user verified automatic box-entry activation,
gray consumption and F4 reset after a real component-test export. Exact Player
body fit and real exported sphere alignment remain open. Automated collision tests
exercise overlap phases, filtering, transforms, and lifetime behavior; they do not
prove visual appearance or Unreal export correctness.
