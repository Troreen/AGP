# InputMapper and ServiceLocator in plain language

This document explains what the two systems do, who owns them, and how input reaches the game.

## The short version

`ServiceLocator` is the engine's shared service cabinet. The runtime puts three services into it during startup:

- `InputMapper`, which turns keyboard, mouse and controller input into named actions.
- `AudioManager`, which controls sound and music.
- `AssetRegistry`, which finds and loads engine assets.

ServiceLocator owns all three objects. When the game closes, ServiceLocator deletes all three. There is no second owner and no separate singleton instance for audio or assets.

`InputMapper` is the middleman between Windows input and game code. Windows reports physical input such as “W is down” or “the mouse moved.” InputMapper translates that into a useful name such as `CameraForward` or `CameraLookDelta`, then tells every listener interested in that name.

## Startup

The runtime performs startup in this order:

1. It creates an `AssetRegistry` and gives ownership to ServiceLocator.
2. It initializes the asset registry with the game's Content folder.
3. It creates an `AudioManager`, gives ownership to ServiceLocator, and initializes audio.
4. It creates an `InputMapper` and gives ownership to ServiceLocator.
5. It connects InputMapper to the runtime-owned `InputHandler` and `XInputHandler`.
6. It installs the game's global key and mouse bindings once.
7. The game and components register listeners for the named actions they use.

The ownership relationship looks like this:

```text
GameApplication runtime
├── owns InputHandler
├── owns XInputHandler
└── uses ServiceLocator
    ├── owns InputMapper
    ├── owns AudioManager
    └── owns AssetRegistry

InputMapper borrows InputHandler and XInputHandler.
```

The two input handlers are not services. They are low-level device objects that belong to the runtime and live longer than InputMapper.

## What happens each frame

Windows messages are first recorded by `InputHandler`. Once per rendered frame, the runtime calls `InputMapper::Update()`.

```text
Windows message
    ↓
InputHandler records the new device state
    ↓
InputMapper updates once for the frame
    ↓
InputMapper translates device input to named actions
    ↓
Registered game and component callbacks run
    ↓
Game and World update
```

For example, the runtime binds W to `CameraForward`. DebugCameraController listens for `CameraForward`. While W is held, its listener records that forward movement is active. The controller then moves the camera during its normal component update using frame time.

## Where bindings belong

Bindings are global configuration, so the runtime installs them once in `GameApplication::InitializeInputAndHostControls`.

Components do not assign keys. A component only listens for action names. This matters because changing a binding in one component would otherwise change it for every other listener in the engine.

The current debug-camera actions are:

| Physical input | Named action |
| --- | --- |
| Right mouse button | `CameraLookEnable` |
| Mouse movement | `CameraLookDelta` |
| W / S | `CameraForward` / `CameraBack` |
| A / D | `CameraLeft` / `CameraRight` |
| Space / Control | `CameraUp` / `CameraDown` |
| F1 | `DebugCamera` |

Game-level controls such as scene reload and the demo chest are also bound once during their owning startup code.

## Listeners and cleanup

`InputMapper::AddEventListener` returns an unsigned listener ID. The object that adds a listener stores that ID. Before the object goes away, it removes the listener with `RemoveEventListener`.

In practice:

- Game removes its listeners in `Game::Shutdown`.
- DebugCameraController removes its listeners in `EndPlay`.
- GameApplication removes its host listeners during service shutdown.

World objects are cleared before ServiceLocator deletes InputMapper. That gives every component a chance to remove its listener while the mapper still exists.

Callbacks should not destroy actors, replace scenes, change bindings, or add/remove listeners while InputMapper is dispatching. A callback should record a small request or update input state, and the normal game or world update should perform the larger change afterward.

## Shutdown

Shutdown runs in the reverse direction from startup:

1. Game shuts down and removes its listeners.
2. World is cleared, causing components to run `EndPlay` and remove their listeners.
3. GameApplication removes its own input listeners.
4. ServiceLocator deletes InputMapper, AudioManager and AssetRegistry.

AudioManager's destructor releases the sound engine. AssetRegistry's normal destruction releases its stored assets. After `KillServices`, the locator contains no service pointers.

## Rules to remember

- Give ServiceLocator heap-allocated services because it takes ownership and deletes them.
- Do not manually delete a service after passing it to ServiceLocator.
- Do not keep using a service pointer after replacing that service or calling `KillServices`.
- Install global input bindings during startup, not inside components.
- Remove every input listener before its callback target is destroyed.
- Keep InputHandler and XInputHandler alive for as long as InputMapper uses them.
