# GameFramework MVP

Start with [the MVP guide](../../Docs/GameFrameworkMVP.md).

Headers sit beside their implementations in five folders:

- Runtime: application loop, context, input and game callbacks.
- World: World, Actor, Component and Transform ownership and lifecycle.
- Components: scene offsets, cameras, lights and meshes.
- Scenes: scene descriptions, assets, property reading and component construction.
- Rendering: the adapter to the existing renderer.

Gameplay uses the headers in Runtime, World, Components and Scenes. Rendering
contains engine implementation details. Includes follow the folder layout, for
example `GameFramework/World/Actor.h`.

The application runs Game::Update, World::Update, then the existing renderer.
World owns Actors; Actors own Components. Components have BeginPlay, Update and
EndPlay. Scene descriptions create registered components through a small property
reader. Scene loading is synchronous.

The original advanced implementation remains on the game-framework branch.
