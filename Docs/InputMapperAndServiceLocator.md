# InputMapper and ServiceLocator in plain language

This document explains what the two systems do, who owns them, and how input
reaches Game and Components.

## Ownership

`ServiceLocator` is the engine's shared service owner and access point. During
startup, Main calls `ServiceLocator::Initialize` to create services including:

- `InputMapper`, which turns device input into named actions.
- `AudioManager`, which controls sound and music.
- `AssetRegistry`, which finds and loads Content assets.
- `EventBus`, `AnimationManager`, `EngineSettings`, and coroutine services.

ServiceLocator owns those objects after they are installed. `KillServices`
deletes the owned services it manages and clears their pointers.
There is no second owner for these services.

GameApplication coordinates input/audio updates and shutdown, but ServiceLocator
owns their storage.

```text
GameApplication
├── owns InputHandler
├── owns XInputHandler
└── coordinates ServiceLocator
    ├── owns InputMapper
    ├── owns AudioManager
    └── owns AssetRegistry

InputMapper borrows InputHandler and XInputHandler.
```

The platform handlers therefore remain alive until after ServiceLocator deletes
InputMapper.

## Startup

Startup follows this order:

1. Main calls `ServiceLocator::Initialize` to allocate services.
2. Main loads settings and initializes assets with the executable-relative Content folder.
3. Main initializes animation and audio, then starts GameApplication.
4. Connect InputMapper to the runtime-owned InputHandler and XInputHandler.
5. Install runtime-wide bindings and listeners.
6. Call `Game::Initialize`, which installs game-owned bindings and listeners.
7. Build the initial World; its Components register their listeners in BeginPlay.

## One input frame

Windows messages first update InputHandler. GameApplication then updates InputMapper
once per frame.

```text
Windows message
-> InputHandler records device state
-> InputMapper translates it to named actions
-> registered callbacks run
-> Game and World perform queued work
```

For example, W maps to `CameraForward`. DebugCameraController listens for that
action, records movement state in its callback, and moves during its Component
update using frame time.

Callbacks should record a small request rather than replacing scenes, destroying
objects, changing bindings, or adding/removing listeners during dispatch. Game or
World update performs the larger operation afterward.

## Who owns each binding

Bindings belong with the system that defines the action.

GameApplication listens to these runtime controls; InputBindings.json supplies their bindings:

| Physical input | Named action | Result |
| --- | --- | --- |
| Escape | `Quit` | Set GameApplication's private quit flag |
| F1 | `DebugCamera` | Toggle the runtime debug camera |
| F5 / F6 | `PreviousRenderPass` / `NextRenderPass` | Change renderer diagnostic view |
| Right mouse button | `CameraLookEnable` | Enable mouse look |
| Mouse movement | `CameraLookDelta` | Update camera look |
| W / S | `CameraForward` / `CameraBack` | Move the debug camera |
| A / D | `CameraLeft` / `CameraRight` | Move the debug camera |
| Space / Control | `CameraUp` / `CameraDown` | Move the debug camera vertically |

Game listens for F4 scene reload because that is project behavior. Key bindings
come from InputBindings.json. Components listen to action names.

Escape is deliberately handled by GameApplication. The Escape callback and
`WM_QUIT` set the same internal state. After `InputMapper::Update`, GameApplication
checks that state and leaves the loop before updating Game, World, audio, or the
renderer. Game and Components have no quit API.

## Listener cleanup

`InputMapper::AddEventListener` returns an unsigned listener ID. The registering
object stores that ID and removes it before the callback target is destroyed.

- Game removes its listeners in `Game::Shutdown`.
- Components such as DebugCameraController remove theirs in `EndPlay`.
- GameApplication removes its Escape and diagnostic listeners during runtime cleanup.

Scene replacement clears the old World while InputMapper still exists, so
Component EndPlay callbacks can unregister safely.

## Shutdown

Shutdown follows the borrowing relationships:

1. Game shuts down and removes its listeners.
2. GameApplication clears the World, which runs Component EndPlay.
3. GameApplication removes its own listeners and releases mouse capture.
4. It clears render references that can retain assets.
5. ServiceLocator releases input, audio, events, assets, animation, settings, and scheduler services.
6. GameApplication destroys the window and its remaining state.

The same order is attempted after exceptions. Cleanup continues through later
phases even if an earlier phase reports an error.

## Rules to remember

- Main calls `ServiceLocator::Initialize` once before using services.
- Do not delete services owned by ServiceLocator.
- Do not keep a service pointer after replacement or `KillServices`.
- Install each global binding once in the runtime or Game that owns the action.
- Remove every listener before its callback target is destroyed.
- Keep InputHandler and XInputHandler alive while InputMapper borrows them.
