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

During M1/M2 only, ModelViewer and host regression fixtures use the explicit
Integration `LegacySceneBridge` to submit old scene recipes. This bridge and the
recipe protocol are removed by M3 in favor of owned data and scene-ID requests.
They are not part of the ordinary gameplay API.

See [the game guide](../../Docs/GameFramework.md) and
[implementation evidence](../../Docs/SimplifiedGameFrameworkImplementation.md).
