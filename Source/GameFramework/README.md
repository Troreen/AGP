# GameFramework layout

GameFramework is the reusable engine layer for the single game in this repository.
Game rules, authored scenes and content catalogs belong in the application project.

```text
GameFramework/
  Runtime/           GameApplication, IGame, GameContext: session lifecycle and facade
    Internal/        GameLoop: engine-only phase timing and fixed-step accumulation
  Input/             GameInput: stable input samples delivered to game callbacks
  World/             World and Actor: entity ownership, transforms and tick dispatch
  Components/        Component base and built-in camera, light and mesh components
  Diagnostics/       Framework logging category
```

Headers and implementations live together. Visual Studio filters mirror these
directories. Include across subsystem boundaries using full framework paths, e.g.
`#include "GameFramework/Runtime/GameContext.h"`; same-directory includes may use
the filename alone. Consumers need `Source` on their include path, not every folder.

## Where to start

- Game authors: `Runtime/IGame.h`, `Runtime/GameContext.h`, then `Components/Component.h`.
- Engine lifecycle work: `Runtime/GameApplication.cpp` and `Runtime/Internal/GameLoop.h`.
- Entity composition and ticking: `World/World.h` and `World/Actor.h`.
- Working game example: `../Application/ModelViewer/ModelViewerScene.cpp` and
  `../Application/ModelViewer/ModelViewerComponents.cpp`.

`Internal` marks implementation details, not a supported game-facing API. It is an
organizational boundary, not compiler-enforced access control. Game code should
use callbacks instead of including the internal scheduler.

## Extending the layout

Add folders when their implementation arrives:

- `Scenes/`: scene descriptions, component registration, loading and reference
  resolution. Keep JSON parsing separate from scene instantiation, and keep
  game-specific component factories registered by the game.
- `Assets/`: reusable asset lookup, importing and caching. The application's
  MeshLibrary currently combines import mechanics with a game-specific catalog;
  only the reusable mechanics should move here.

Built-in components stay under `Components`; game-authored components stay in the
application. GPU resources and rendering remain in GraphicsEngine. Avoid turning
Runtime into a miscellaneous helpers folder. The layout does not remove existing
framework/renderer dependencies or introduce a scene loader by itself.
