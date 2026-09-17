# GameFramework

Gameplay uses `GameApplication`, `IGame`, `GameContext`, `World`, `Actor` and
`Component`. The host owns frame advancement and lifecycle. Implement game hooks,
spawn actors, add components, and keep checked `ActorRef`/`ComponentRef<T>` values
when storing references between callbacks.

The supported core include root is `Source/GameFramework/Public`, together with
`CommonUtilities/include`. Include `<GameFramework/IGame.h>` and
`<GameFramework/World.h>`, for example. All public headers compile without GraphicsEngine, D3D, platform or private include directories.

`IGame::RegisterComponents` registers game types once. The engine registers its
`agp.*` built-ins first and freezes registration before Initialize. A minimal game
can compose its bootstrap world directly in Initialize. Components start
successfully before their first tick; game code never prepares, activates, flushes,
or advances the world manually.

Implementations and engine-only access live in Private. Integration contains the source/data/ready-resource boundary. Public contains actual supported definitions; temporary forwarding headers and legacy API aliases are removed. World construction and lifecycle are host-only.

SceneService accepts Load/Reload by SceneId. Integration/ISceneSource supplies owned
SceneData and ready assets; the engine allocates registered types, reads checked
properties, resolves IDs and validates before beginning. Rejected replacements
preserve the current world in Debug and Release. LegacySceneBridge and executable
scene recipes have been removed. ModelViewerScene is the temporary C++ source;
real Perforce integration waits for verified importer contracts and fixtures.

See [the game guide](../../Docs/GameFramework.md) and
[implementation evidence](../../Docs/SimplifiedGameFrameworkImplementation.md).
