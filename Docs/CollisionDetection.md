# Collision detection

Collision detects nonblocking overlaps between spheres and yaw-rotated boxes.
Gameplay decides what Enter, Stay, and Exit mean. There is no movement response,
combat rule, importer integration, or continuous/swept detection in this version.

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
overlap callbacks, using the collider's current composed transform. Disabled
colliders use the disabled color; inactive Actors and destroyed colliders do not
draw. Colors and the toggle have matching getters. Outlines show authored geometry;
an unsupported transform can still be outlined while detection excludes it.
The renderer clears these temporary lines each rendered frame.

## Subscribe from gameplay

Include `GameFramework/Collision/CollisionOverlapEvent.h`,
`GameFramework/EventBus.hpp`, and `GameFramework/ServiceLocator.h`. A gameplay
Component retains its collider ID and the returned subscription token as a
`std::uint32_t` member (include `<cstdint>` in that Component's header):

```cpp
void DetectionListener::BeginPlay()
{
    myListenerId = ServiceLocator::GetInstance().GetEventBus()
        .AddEventListener<CollisionOverlapEvent>([this](const CollisionOverlapEvent& event)
        {
            if (event.SourceWorld != &GetWorld() ||
                (event.FirstColliderId != myColliderId && event.SecondColliderId != myColliderId))
            {
                return;
            }
            // React to event.Phase: Enter, Stay, or Exit.
        });
}

void DetectionListener::EndPlay() noexcept
{
    ServiceLocator::GetInstance().GetEventBus().RemoveEventListener(myListenerId);
}
```

The runnable example is `Components/CollisionDemoComponent` in the Game project.
It subscribes in BeginPlay, checks both the World and moving collider ID, and
unsubscribes in EndPlay. Scene replacement uses the same cleanup path.

## Timing and lifetime

Detection follows normal Component updates using the World's frozen frame
membership. Components added during callbacks or updates wait for the next
eligible frame. Touching counts as overlap. One unordered pair produces Enter
on its first sampled overlap, Stay on later overlapping updates, and Exit after
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

The integration uses the existing EventBus unchanged. Subscriptions belong to the
listening Component: add them in BeginPlay and remove them in EndPlay.

Detection samples frames. A fast volume may cross another entirely between
updates without an event. Direct queries, capsules, compound colliders, blocking,
and physical resolution are outside this version.

## Run the optional demonstration

1. In `Source/Application/Game/Game.cpp`, set the single code switch
   `EnableCollisionDemo` to `true` and build Debug x64. Its checked-in default is
   `false`; no settings or input bindings need changing.
2. Run `Bin/Debug/Game.exe`. The demonstration uses whichever scene the existing
   application settings select. For the recorded verification procedure use
   `lvl_verify_anim/Lvl_Verify_Anim_Level.json`, if present in the local Content
   stream. The four separate volume Actors are spawned through ConfigureWorld.
3. Keep the initial camera still for the 20-second run. At BeginPlay the sample
   places its center 650 units along that camera's full view direction. Movement
   uses a horizontal lateral axis; the camera itself is unchanged. Normal scene
   geometry can occlude depth-tested outlines; use the existing camera controls
   to inspect them if needed.
4. Compare outlines and Debug console messages with the table below. The moving
   ID must appear with the fixed sphere and box, never with the filtered ID.
   Stay is delivered every overlapping frame but logged once per pair entry.
5. Press the existing F4 reload action during an overlap. Expect one
   `Collision demo EndPlay: listener removed.` message, a fresh start, and one
   set of phase logs. Clear emits no old-world Exit events. Repeat the run to
   check that old listeners do not accumulate.
6. Restore `EnableCollisionDemo` to `false` and rebuild after the check.

| Time | Expected observation |
| --- | --- |
| 0–4 seconds | Cyan sphere approaches yellow fixed sphere, then Enter and Stay. |
| 5 seconds | Cyan becomes gray while overlapping; disable log and Exit. |
| 6 seconds | Cyan returns; Enter and Stay occur again. |
| 7 seconds onward | Moving sphere leaves the fixed sphere; Exit. |
| About 10–14 seconds | Cyan crosses magenta layer-2 sphere with no event for its ID; the layer-1 box may also enter range toward the end. |
| By 15 seconds | Cyan rests inside the yellow yaw-rotated box; Enter and Stay. |
| 16 seconds | Box Actor is destroyed; its outline disappears and Exit occurs, with null lookup allowed. |
| 17–20 seconds | Cyan moves away, then the completion log appears. F4 starts another run. |

The scene, outlines, phase order, enable/disable, destruction, and reload cleanup
need runtime observation. Build success alone does not verify those checks.
