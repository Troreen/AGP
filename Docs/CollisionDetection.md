# Collision detection

Collision detects nonblocking overlaps between spheres and yaw-rotated boxes.
Gameplay decides what Enter, Stay, and Exit mean. There is no movement response,
combat rule or continuous/swept detection in this version. The Unreal scene importer
supports exported box and sphere components.

## Spawn and configure

Include `GameFramework/Components/ColliderComponent.h` and
`GameFramework/World/World.h`. Each independent volume needs its own Actor:

```cpp
Actor* actor = world.SpawnActor("DetectionVolume");
auto* collider = actor->AddComponent<ColliderComponent>("Collider");
collider->SetSphereRadius(100.0f); // Or SetBoxHalfExtents({100, 50, 80}).
actor->GetTransform().SetWorldPosition({0, 100, 0});
collider->GetTransform().SetLocalPosition({0, 20, 0}); // Actor-relative offset.
collider->SetLayer(3);
collider->SetMask((1u << 0) | (1u << 4));
const ColliderId id = collider->GetColliderId(); // Available immediately after attachment.
```

One non-destroyed collider is allowed per Actor, even if disabled. Adding another
throws `std::invalid_argument`. Shape and layer setters return false for invalid
input and preserve their previous configuration. Sphere radius and box
half-extents must be finite and positive. Boxes support positive nonuniform scale
and world-up yaw without shear; spheres require positive uniform scale.
Unsupported transforms are diagnosed and excluded from detection for that pass.

Layers are numbered 0–31. The default is layer 0 with every mask bit enabled.
Both masks must accept the other collider's layer. A zero mask accepts nothing.
Layer meanings belong to gameplay. All layer combinations remain nonblocking.

## Import shapes from Unreal

Use a Box Collision or Sphere Collision component. The existing exporter provides
box `Bounds` (half-extents), sphere `Radius`, component `Transform`, and `Tags`.
AGP creates a `ColliderComponent` from that data without changing the exporter.
Dimensions remain local; the Actor and component transforms apply scale once.
The shape can be the Actor root or attach directly to its root. Deeper attachment
chains, multiple supported shapes on one Actor, and invalid dimensions produce
import errors. Capsule components remain placeholders.

Use the shape component's Tags to choose filtering:

```text
CollisionLayer=Trigger
CollisionMask=Player
```

The game supplies these exact, case-sensitive names to the importer:

| Name | Layer |
| --- | --- |
| Player | 1 |
| Trigger | 2 |
| Enemy | 3 |

Separate several mask names with `|`, for example
`CollisionMask=Player|Enemy`. `CollisionMask=` explicitly accepts nothing.
Each setting can appear once. Unknown names, duplicate settings, and malformed
settings produce import errors.

Missing tags preserve the existing defaults: enabled, layer 0, all mask bits set.
Layer 0 has no authoring name in this increment. Both masks must still accept the
other collider's layer: a Player that must enter a Trigger needs Trigger in its
mask, and the Trigger needs Player in its mask.

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
draw. Colors and the toggle have matching getters. Outlines show authored geometry;
an unsupported transform can still be outlined while detection excludes it.
The renderer clears these temporary lines each rendered frame.

## Subscribe from gameplay

Include `GameFramework/Components/ColliderComponent.h`. Register directly on
the collider you want to observe. Keep the returned `CollisionSubscription` as a
member of your listening Component:

```cpp
void DetectionListener::BeginPlay()
{
    auto* collider = GetOwner()->GetComponent<ColliderComponent>();
    myEnterSubscription = collider->OnEnter(this, &DetectionListener::OnEnter);
}

void DetectionListener::OnEnter(const Collision& collision)
{
    if (Actor* other = collision.OtherActor())
    {
        // Apply your gameplay rules to the entering Actor.
    }
}

void DetectionListener::EndPlay() noexcept
{
    myEnterSubscription.Reset();
}
```

`OnStay` and `OnExit` use the same signature. Lambdas are also supported:
`myEnterSubscription = collider->OnEnter([this](const Collision& collision) { /* gameplay */ });`.
Member-function receivers must be Components; callbacks skip receivers that have
not begun play, are disabled, or have inactive/destroyed owners. Keep each
subscription on its receiver and call `Reset()` in `EndPlay`. Destroying or replacing
a subscription also disconnects it. Lambda captures must live as long as their subscription.

`collision.OtherCollider()` and `collision.OtherActor()` look up the other participant;
either can return null after destruction. `OtherColliderId` remains available even
when lookup returns null. `Phase` identifies Enter, Stay, or Exit when sharing one
handler. The collision and returned pointers are borrowed for the callback only.

Callbacks run on both surviving colliders. New subscriptions wait until the next
notification on that collider; disconnecting skips a callback that has not run yet.
The gameplay example is `ScriptedEventComponent`: its optional `TriggerCollider`
names a collider on the same Actor. It stores an Enter
subscription and resets it in EndPlay, including on scene replacement.

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
and listeners that need every pair. Collider callbacks run before that pair's
EventBus event. Gameplay normally uses the collider callback API above, which
handles World filtering and pair IDs internally.

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
