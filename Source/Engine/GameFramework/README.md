# GameFramework MVP

Start with [the MVP guide](../../Docs/GameFrameworkMVP.md).

GameFramework contains the reusable gameplay data and object model:

- World: World, Actor, Component and Transform ownership and lifecycle.
- Components: scene offsets, cameras, lights and meshes.
- Scenes: `SceneData`, property reading, and `BuildWorldFromSceneData`.
- Rendering: the adapter that copies a live World into a render snapshot.
- AssetHandling, audio, input and ServiceLocator: the shared engine services.
- UnrealSceneImporter: conversion from the exported Unreal JSON format.

The application-specific loop lives in `Source/Application/Game/GameApplication.*`.
It calls the concrete `Game` directly. Scene requests and the live World stay
inside GameApplication.

`GameApplication` coordinates each frame in this order: input, `Game::Update`,
`World::Update`, audio, snapshot construction, and rendering. It owns the current
World. World owns Actors, Actors own Components, and Components receive
`BeginPlay`, `Update`, and `EndPlay`.

Scene loading is synchronous. Application code imports and prepares `SceneData`,
then `BuildWorldFromSceneData` creates a candidate World. The runtime commits that
World only after construction and `Game::ConfigureWorld` succeed.
