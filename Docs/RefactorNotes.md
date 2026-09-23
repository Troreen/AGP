`SceneData` currently lives in `GameFramework` which feels backwards considering that is game specific data. It should be moved to the `Game` module. 

`GameScene` should be renamed to something like `SceneLoader` since it isnt a scene by itself it is a helper that loads scenes.

Rename `IGame` into soemthing more explanatory like `GameInterface`


Overall architecture needs to be rethought. There is too much abstraction and things made for "reusablity" this just creates a lot of complexity and makes it hard to understand what is going on. The engine will not need these kinds of features given our 3 game scope. Things that need to be changed from game to game can be done so manually. 

instead of exposing the internal layers of scene loading we should hide it behind a simple SceneLoader

Component regisrty is redundant and can be put inside SceneLoader


# Final plan
The Refactor should aim for **fewer layers, clearer ownership, and a more direct path from `Main` -> game -> runtime -> engine systems.**

- Remove abstractions that only exist for hypothetical reuse. The biggest candidates are `IGame`, much of `GameContext`, and `SceneSource`, and possibly the `GameApplication::Impl` shell. This engine only needs to run one game, so the architecture should reflect that. 
- Make the runtime concrete and obvious. One runtime/applicatin object should own the main loop, window, timing, current `World`, scene transitions, and coordination of graphics/input/audio rather than passing everything everything through interfaces and context objects.
- Keep `ServiceLocator` as the intentional engine-wide service access point.
- Simplify scene loading. Replace the current `SceneSource` -> `GameScene` -> `SceneData` -> `ComponentRegistry` -> `World` chain with a cearer concrete scene-loading path. SceneData can remain as teh useful serialized/intermediate representation, but the runtime should not need to understand everey step of contructing it.
- Remove fake "registries" and wrapper classes where they only namespace one operation.