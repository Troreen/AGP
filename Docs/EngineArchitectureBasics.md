  # Engine architecture, the simple version

  This doc deliberately skips the details of the graphics engine. The graphics side is
  important, but you do not need to understand it before adding an Actor, loading a
  scene, or writing a gameplay Component.

  ## The whole thing in one picture

  ```text
  Main
    -> GameApplication starts the engine
    -> Game asks for a scene
    -> GameScene reads the exported file
    -> ComponentRegistry builds a World
    -> Game adds any code-only behavior
    -> World calls BeginPlay on the Components
    -> the normal frame loop starts
  ```

  That is most of the engine from the game's point of view.

  ## The main pieces

  ### GameApplication

  `GameApplication` is the host. It creates the window, starts the main services,
  loads scenes, runs the frame loop, and shuts everything down again.

  Game code normally does not need to edit it. It should feel a bit like the stage
  manager: it gets everything into place and then calls the right people at the
  right time.

  ### Game and GameContext

  `Game` is our project-specific entry point. This is where we request the first
  scene and add behavior that belongs to this game rather than to the reusable
  engine (aka. `GameFramework`).

  `GameContext` is the small bundle passed to the game callbacks. It gives the game
  access to the current World, input, scene loading, reload, and quit requests.

  A scene request is queued. It is not loaded halfway through whatever the World is
  currently doing. `GameApplication` picks it up at a safe point.

  ### GameScene and the importer

  `GameScene` knows which scene name maps to which file. For example:

  ```text
  ChestMaterials -> Content/ExportedScenes/ChestMaterials_Level.json
  ```

  The Unreal importer reads that JSON and turns it into plain `SceneData`. At this
  point it is still only a description. There are no live Actors or Components yet.

  `GameScene` also connects the game's FBX loader and checks material references.
  Materials that exist locally are kept. Old Unreal-only material names use the
  simple fallback material instead.

  ### AssetRegistry

  The asset registry is the project's shared asset cupboard.

  It indexes files below `Content`, understands paths without caring about letter
  case, loads an asset when it is first needed, and reuses it while something still
  holds it. Meshes, materials, and textures all go through the same place.

  Gameplay code receives small asset handles. It does not need to know how the
  renderer stores the real resource.

  ### World, Actor, and Component

  The ownership rule is pleasantly small:

  ```text
  GameContext owns one World
  World owns its Actors
  Actor owns its Components
  ```

  An Actor is mainly a name, tags, a transform, and a place to attach Components.
  Components provide the actual behavior or purpose: mesh, camera, light, spinning,
  input handling, and so on.

  Every Actor has a transform. A `SceneComponent`, such as a light or mesh, also has
  its own local transform. That local transform is relative to the Actor. This is
  why rotating one Actor can move an offset light around it without manually moving
  the light each frame.

  ### ComponentRegistry

  The component registry is the builder between `SceneData` and a live `World`.

  It creates each Actor, creates the requested Components, applies transforms and
  properties, and resolves the assets used by mesh Components.

  The new World is built off to the side. If the scene itself is malformed, the
  currently running World is left alone. If one mesh asset is simply missing, only
  that mesh Component is skipped and a warning is logged.

  ### Input, audio, and ServiceLocator

  Input is collected by the host and turned into named actions such as reload,
  quit, or toggle spin. Components subscribe to the actions they care about.

  Audio is updated once per frame by the host.

  `ServiceLocator` is the current common doorway to general services such as
  Input, Audio, and Assets. It is useful when a Component needs a service but should
  not own it. The locator only points at these services; it does not control their
  lifetime.

  ## Following the real program to BeginPlay

  This is the path taken by the actual Game executable. The runtime tests are
  separate programs which exercise parts of the same path; they are not where the
  game starts.

  ### 1. Program start

  Windows enters `wWinMain` in `Main.cpp`. It finds the executable's location and
  uses that to point the engine at the `Content` folder. It then creates three
  things:

  - the project's `Game`,
  - a `GameScene`,
  - and a `GameApplication` configuration pointing at `Content`.

  It calls `GameApplication::Run` and gives it a small function which forwards any
  scene request to `GameScene::Load`.

  ### 2. Basic services start

  `GameApplication` creates the window and starts the engine systems. After the
  graphics setup, it indexes the Content directory, starts audio, installs the input
  bindings, and puts Input, Audio, and Assets into `ServiceLocator`.

  ### 3. The game requests the scene

  Once those services are ready, `GameApplication` calls `Game::Initialize`.
  The game installs its quit and reload controls, then calls:

  ```cpp
  context.LoadScene("ChestMaterials");
  ```

  This only records the request. After `Initialize` returns, the host sees the
  pending request and begins loading it. The starting scene is therefore chosen by
  the game project, not by a test runner or by the engine itself.

  ### 4. The exported JSON becomes SceneData

  The scene-loading function passed in by `wWinMain` receives the name and calls
  `GameScene::Load("ChestMaterials")`. `GameScene` selects
  `Content/ExportedScenes/ChestMaterials_Level.json`.

  The importer reads the Unreal-shaped records and produces five Actor descriptions:

  - three chest Actors with different materials,
  - `SunLight` with a directional light,
  - `DoubleLight` with two point lights.

  `CenterPointLight` has a local position of `(0, 0, 0)`. `OrbitPointLight` has an
  exported offset of 250 Unreal centimetres, which becomes 2.5 local engine units.

  ### 5. The live World is built

  `ComponentRegistry` creates a fresh candidate World and fills it from those
  descriptions. Calling it a candidate matters: the currently running World is not
  disturbed if the new scene cannot be constructed.

  The chest mesh is requested from `AssetRegistry`, and the material in each
  exported slot is turned into a live material instance.

  The two lights on `DoubleLight` are separate `PointLightComponent` objects owned by
  the same Actor. Their local transforms are kept exactly as authored.

  ### 6. Game code adds the behavior

  The exported scene describes what exists. Game code adds what it does.

  Before the World starts, `Game::ConfigureWorld` finds `DoubleLight` and attaches
  the existing spin behavior:

  ```cpp
  if (Actor* doubleLight = world.FindActor("DoubleLight"))
  {
      doubleLight->AddComponent<SpinComponent>("Spin");
  }
  ```

  This is a useful pattern for small bits of project logic: author the objects and
  their values in the scene, then attach reusable game behavior in code.

  ### 7. The World becomes current

  Only after construction and `Game::ConfigureWorld` succeed does
  `GameApplication` replace the old World. It resets the scene input state and, if
  the scene has no camera, adds its small debug camera so the scene can still be
  viewed.

  ### 8. BeginPlay

  Finally, `World::BeginPlay` walks over the Components that exist at that moment and
  calls `BeginPlay` once on each one.

  For this scene, the interesting call is `SpinComponent::BeginPlay`. It subscribes
  to the Toggle Spin input action, which is bound to `R` in the sample controls.

  After every Component has had its `BeginPlay`, the game receives
  `OnSceneLoaded`. The current `Game` does not need to do anything extra there. The
  scene is now live and the main frame loop begins.

  On each following frame, the host processes window messages and input, calls
  `Game::Update`, and then calls `World::Update`. That reaches
  `SpinComponent::Update`. The Component rotates the `DoubleLight` Actor. The
  center light stays at the Actor's
  origin, while the offset light moves around it because its local transform is
  combined with the Actor transform.

  When the program closes, `Game::Shutdown` releases its input subscriptions and
  the World is cleared. Clearing the World gives live Components their matching
  `EndPlay` call before the shared services are shut down.

  ## Where I would add something new

  - A new level object or starting value: add it to the exported scene.
  - A reusable action that runs every frame: write a Component.
  - A project-specific connection between scene objects: use `Game::ConfigureWorld`.
  - A new mesh, material, or texture reference: put it under `Content` and use its
    Content-relative path.
  - A new input: define an action, bind it once, and subscribe from the Component
    that owns the behavior.

  When in doubt, keep scene data as data and keep behavior in Components. That rule
  fits nearly everything in the current engine.
