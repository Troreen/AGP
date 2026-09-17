# GameFramework

Gameplay uses `GameApplication`, `IGame`, `GameContext`, `World`, `Actor` and
`Component`. The host owns frame advancement and lifecycle. Implement game hooks,
spawn actors, add components, and keep checked `ActorRef`/`ComponentRef<T>` values
when storing references between callbacks.

The supported core include root is `Source/GameFramework/Public`, together with
`CommonUtilities/include`. Include `<GameFramework/IGame.h>` and
`<GameFramework/World.h>`, for example. Core headers compile without GraphicsEngine,
D3D, platform or private include directories. Renderer component isolation and the
physical public/private file move complete in milestone M4.

`IGame::RegisterComponents` registers game types once. The engine registers its
`agp.*` built-ins first and freezes registration before Initialize. A minimal game
can compose its bootstrap world directly in Initialize. Components start
successfully before their first tick; game code never prepares, activates, flushes,
or advances the world manually.

Engine implementation currently lives in Runtime, World, Components and Scenes.
`Runtime/Internal` access helpers are for the host, construction and CPU tests.
Lifecycle, ownership collections and input bookkeeping are private, independently
of folder placement. `Scenes` is implemented; it is not a future directory.

SceneService accepts Load/Reload by SceneId. Integration/ISceneSource supplies owned
SceneData and ready assets; the engine allocates registered types, reads checked
properties, resolves IDs and validates before beginning. Rejected replacements
preserve the current world in Debug and Release. LegacySceneBridge and executable
scene recipes have been removed. ModelViewerScene is the temporary C++ source;
real Perforce integration waits for verified importer contracts and fixtures.

See [the game guide](../../Docs/GameFramework.md) and
[implementation evidence](../../Docs/SimplifiedGameFrameworkImplementation.md).
