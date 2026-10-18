# Collision system investigation

Status: resolved

## Current scope (2026-10-17)

The broad collision/backend investigation below is historical. The authoritative v1 is [spec.md](spec.md): general code-spawned sphere/yaw-only-box overlap detection, one ColliderComponent per Actor, a World-owned CollisionSystem, and CollisionOverlapEvent notifications through the existing EventBus. Each collider has one of 32 numbered layers and a bilateral detection mask; gameplay owns layer meanings. The default is layer 0 with all layers allowed. Combat is not the design basis. Blocking, projectiles, capsules, navigation integration, direct queries, and Unreal tag/import work are deferred. No external physics library is needed for this scope. Multiple-collider suggestions and backend recommendations below are historical, not current requirements.

Correction from combat investigation: Root-Issue does expose a pairwise CollisionQuery API in CollisionQuery.h/.cpp. CombatSystem creates temporary sphere/axis-aligned-box attack shapes, checks targets over an active window, and tracks already-hit target IDs. It directly applies damage/knockback and collects HitEvent records rather than publishing EventBus events. The query geometry duplicates part of RuntimeCollisionSystem; sphere-versus-oriented-box uses the enclosing AABB and unhandled query pairs fall back to AABB separation. This is inspiration for keeping AGP's overlap geometry in one internal implementation, not a reason to add combat or public queries to v1.

## AGP facts verified in this checkout

| Evidence | Consequence |
| --- | --- |
| `Source/Engine/GameFramework/World/World.cpp`: cleanup, frozen component list, BeginPlay, enabled updates; owned Actors and Components | Collision belongs to one World, respects existing lifecycle, and must not retain dangling pointers through cleanup. |
| `Components/SceneComponent.cpp`: local matrix composed with Actor matrix | Collider offsets reuse SceneComponent transforms. Do not invent offset/pivot infrastructure or assume world-axis alignment. |
| `Source/Application/Game/Player/PlayerMovementComponent.cpp`: direct destination movement writes local position | Blocking requires an explicit movement integration; attaching colliders alone cannot block the player. |
| `PlayerController.cpp`: screen ray intersects Y=0 plane; navmesh pathfinding TODO | Multi-height click targeting and navigation height are not implemented by this code path. Keep the navigation dependency visible. |
| `AssetHandling/NavMeshAsset.h/.cpp`, `Source/Engine/AISystems/Pathfinder.hpp` | Existing navmesh/pathfinding work is the starting point for navigation; collision must not duplicate it. No working height-following player integration was found. |
| `Scenes/SceneData.h`, `SceneWorldBuilder.cpp`, `UnrealSceneImporter.cpp` | Box/sphere/capsule dimensions survive import as placeholder data. Builder currently creates placeholder Components. Importer reads Bounds/Radius/HalfHeight but no dedicated collision response schema in those cases. |
| `Source/Utilities/CommonUtilities/Intersection.hpp` | Existing ray, plane, AABB, and sphere predicates are reusable after edge-case checks. They do not by themselves supply continuous movement response. |
| `Tests/GameFramework/GameFrameworkTests.cpp`, test projects and header-isolation script | Prefer externally observable World/Component/query tests. No need for a new test framework. |

Inspected exported scene counts using JSON TypeID groups: blockout has no box/sphere/capsule records; component-test has none; top-down has one capsule and 332 static meshes. This is a three-scene sample, not an audit of all content. Visual meshes cannot silently become colliders. Collision authoring remains a real requirement.

## Historical reference: conceptual reuse and improvements

Paths below are relative to `C:/Users/tarik/Documents/GitHub/Root-Issue-Game/Source/Game/source`.

- `RuntimeCollisionSystem.cpp` separates bounding boxes from shape-specific tests and implements box, sphere, capsule, and oriented-box separation. Reuse the broad/narrow distinction and focused geometry helpers conceptually; support only approved shape combinations.
- `CollisionTypes.h` names Enter/Stay/Exit and separates Ignore/Block/Trigger. Clear concepts, but detection, response, and gameplay effects should have separate owners.
- Runtime pass refreshes shapes after movement and bounds penetration iterations. Good reasons to make spatial state current and bound correction work.
- `RuntimeCollisionSystem::Run` groups by hard-coded Player/Enemy/WorldStatic/etc. and mutates positions. Avoid game-specific classes/rules in AGP's engine service.
- `RefreshRuntimeCollider` calls component Update(0). Prefer explicit shape synchronization or direct shape derivation; normal behavior updates should not be a geometry refresh API.
- `MakeCollisionPairKey` stores a mixed hash alone as the key. Hash equality is not pair equality. Retain the actual ordered ID pair and compare both IDs.
- Shape selection takes the first matching collider type on an object. AGP should identify collider instances, allowing separate body, trigger, and attack volumes on one Actor without implicit priority.
- Trigger components expose one inside boolean; `StateStack/GameState_InGame.cpp` maps both Enter and Stay to OnTriggerEnter. Avoid ambiguous naming and loss of per-occupant information.
- Exit contacts are skipped if either object is absent from the current live map. AGP needs an explicit destruction/deactivation contract rather than assuming every separation generates an Exit with two live pointers.
- Existing reference pass checks overlap and corrects penetration after movement. It is not evidence of safe fast-projectile sweeps; copying it would not satisfy that requirement.
- Large per-frame logging sections obscure the main collision flow. Reuse AGP logging and LineDrawer with a small opt-in debug view.

## Historical comparison: custom versus library

This is a comparison, not a backend selection. Jolt is a representative candidate, not an exhaustive library survey.

| Option | Benefit for AGP | Cost / restriction |
| --- | --- | --- |
| Custom primitive collision | Small dependency footprint; algorithms and data flow remain visible; reuse CU math | Exact rotated-shape sweeps, moving-target hits, penetration recovery, and sliding become our maintenance responsibility. Must agree a deliberately restricted shape matrix. |
| Jolt behind one concrete AGP collision service | Existing ray/overlap/shape-cast APIs and broad phase reduce geometry algorithm work | Build dependency, initialization/cleanup, shape/body lifecycle mapping, units and coordinate conversion, filtering, and synchronization still need AGP integration. Not automatically less engine code for a tiny primitive-only game. |

Official [Jolt collision-query documentation](https://jrouwe.github.io/JoltPhysicsDocs/5.5.0/index.html) describes ray queries, static shape overlaps, and shape casts. These support the required kinds of queries; AGP movement policy and damage rules remain our responsibility.

The official [HelloWorld source](https://github.com/jrouwe/JoltPhysics/blob/master/HelloWorld/HelloWorld.cpp) demonstrates allocator/type initialization and object cleanup. The [build guide](https://github.com/jrouwe/JoltPhysics/blob/master/Build/README.md) documents Windows/MSVC/CMake integration and configuration options. This is documentation review only: no dependency downloaded, pinned, compiled, or benchmarked, and no claim of verified AGP v145 compatibility.

The same query documentation explains valid-scale restrictions and shear concerns. Regardless of backend, define supported scale/rotation combinations explicitly instead of silently enlarging boxes or changing sphere geometry.

Recommendation for discussion: if authored primitive volumes and a restricted sweep matrix cover the actual levels, custom is credible. If rotated collision, richer geometry, and reliable general sweeps are required, prefer evaluating Jolt before committing to custom narrow-phase work. Navigation supplying height removes the need to adopt a character controller merely to climb stairs.
