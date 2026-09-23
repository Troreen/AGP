# Importer handoff for the MVP

The newer Perforce importer is not in this checkout. Do not infer its schema,
coordinate system, or material conventions from the current sample export.

The stable data boundary is
`Source/Engine/GameFramework/Scenes/SceneData.h`. An importer returns owned
`ActorRecord` and `ComponentRecord` values in a `SceneData`. It must not create
Actors, call lifecycle methods, or depend on GameApplication.

The current concrete path is:

```text
GameApplication captures SceneId
-> GameApplication selects the exported scene file
-> UnrealSceneImporter converts the source file
-> SceneData value
-> BuildWorldFromSceneData(SceneData, AssetRegistry, client size, fallback assets)
-> candidate World
```

SceneId-to-file mapping and fallback asset selection live in GameApplication
because they are project policy.
`BuildWorldFromSceneData` lives in GameFramework because it maps the engine's
fixed SceneData variants to live engine Components.

The importer should report useful diagnostics and return plain data. Empty
SceneData is a valid empty scene. Asset resolution belongs after source-format
conversion; use the existing AssetRegistry rather than creating another asset
owner or importer-specific service.

Before writing an adapter, agree with the importer team on:

- Axis orientation, units, quaternion order, and conversion ownership.
- Actor transforms in world space and Component offsets relative to the Actor.
  Nested hierarchies are deferred; flatten them explicitly or reject them.
- Unique Actor names and per-Actor Component names.
- Supported Component type names and scalar, vector, and asset properties.
- Material slot ordering and asset identifier mapping.
- How source errors and unsupported records become importer diagnostics.
- Representative valid, malformed, and unsupported export fixtures.

The current `PropertyValue` supports bool, integer, double, string, Vector3,
AssetId, and lists of AssetId. It is a narrow interchange type rather than a
general serialization or reflection system.
