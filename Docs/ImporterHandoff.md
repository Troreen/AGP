# Importer handoff for the MVP

The newer Perforce importer is not in this checkout. Do not infer its schema,
coordinate system or material conventions from the current C++ sample.

The boundary is `Source/Engine/GameFramework/Scenes/SceneData.h`.
A SceneSource callback returns owned ActorRecord and ComponentRecord values.
The registry builds runtime objects and applies properties; the importer must
not create Actors, drive lifecycle or reference private engine implementation.

The callback receives a content root, client size and AssetLibrary. Use the
existing asset backend to bind ready meshes/materials, then refer to those
bindings with AssetId properties. Throw a useful exception when loading fails;
empty SceneData is a valid empty scene.

Before writing an adapter, agree with the importer team on:

- Axis orientation, units, quaternion order and conversion ownership.
- Actor transforms in world space and component offsets relative to the Actor.
  Nested hierarchies are deferred; flatten explicitly or reject them.
- Unique Actor names and per-Actor component names.
- Registered component type names and supported scalar/vector/asset properties.
- Material slot ordering and asset identifier mapping.
- Representative valid, malformed and unsupported export fixtures.

The current PropertyValue supports bool, integer, double, string, Vector3,
AssetId and lists of AssetId. It is deliberately not a universal serialization
system. The advanced importer/diagnostic design remains on game-framework.
