# GameFramework: a guided gameplay code tour

Keep this guide open beside Visual Studio. Open the linked files and navigate to the named functions. Each stop gives you the code to show, the explanation to give, and the point teammates should take away. Show the actual implementation in the editor.

The example is **OrientationGizmos_TGE** with a **SpinComponent** attached in C++. The programmer confirmed that it rotates. Earlier testing confirmed Cube2 rotation and F4 reload without a crash. Unreal custom-component export and the game-owned factory remain TODOs.

Allow about 15–20 minutes for the tour, with questions and demonstrations taking it toward 20–30 minutes. Keep the initial engine-loop overview to about two minutes. Focus the remaining time on implementing gameplay.

## Before presenting

- Build Game and select Blockout in the **current** initial-scene setting in [ApplicationSettings.json](../Bin/Settings/ApplicationSettings.json).
- Run the scene, locate the orientation gizmo, and rehearse F4 reload.
- Open the files below in editor tabs and keep this guide in Markdown preview beside them.

| Stop | File | Function or declaration |
| --- | --- | --- |
| 1 | [Main.cpp](../Source/Application/Game/Main.cpp), [GameApplication.cpp](../Source/Application/Game/GameApplication.cpp) | wWinMain, RunSession, RunMainLoop |
| 2 | [Game.cpp](../Source/Application/Game/Game.cpp) | Game::ConfigureWorld |
| 3 | [SpinComponent.h](../Source/Application/Game/SpinComponent.h) | SpinComponent |
| 4 | [Actor.h](../Source/Engine/GameFramework/World/Actor.h), [Component.h](../Source/Engine/GameFramework/World/Component.h) | Attachment, lookup, ownership |
| 5 | [SpinComponent.cpp](../Source/Application/Game/SpinComponent.cpp) | BeginPlay, FindTargetTransform, Update |
| 6 | [World.cpp](../Source/Engine/GameFramework/World/World.cpp) | BeginPlay, Update |
| 7 | [GameApplication.cpp](../Source/Application/Game/GameApplication.cpp), [WorldFromSceneData.cpp](../Source/Engine/GameFramework/Scenes/WorldFromSceneData.cpp) | Scene construction and activation |
| 8 | Game folders and engine components | Integration choices for team features |
| 9 | [UnrealSceneImporter.cpp](../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp), [SceneData.h](../Source/Engine/GameFramework/Scenes/SceneData.h) | Unfinished custom-component path |

## Show the result first — 1 minute

**Show:** the orientation gizmo rotating in Blockout.

**Explain:** the scene supplies the actor and mesh; our component supplies behavior. We will follow that feature through the files so teammates can repeat the process.

**Clarify:** today's attachment happens in Game::ConfigureWorld. It is not yet created from an Unreal-authored custom component.

## 1. Brief engine-loop overview — 2 minutes

### Main.cpp → wWinMain

**Show:** construction of Game and GameApplication, then application.Run(game).

**Explain:** GameApplication coordinates the run. Game contains project-specific choices and behavior. Usually we add gameplay through Game and components rather than editing the window or rendering loop.

Skip the detailed Windows setup, service initialization, and error handling.

### GameApplication.cpp → RunSession

**Show:** InitializeGameSession followed by RunMainLoop near the end.

**Explain:** startup prepares the application and starts the initial World before entering the frame loop. We will return to scene construction when it explains where our component is attached.

### GameApplication.cpp → RunMainLoop

**Show:** input update, aGame.Update, myWorld->Update, audio, scheduler updates, and RenderFrame.

Use this short overview while pointing to the corresponding calls:

    Messages / scene requests / resize / timing
        → InputMapper and application controls
        → Game.Update
        → World.Update — our components run here
        → audio and coroutine scheduler
        → render snapshot, rendering, and present

**Explain:** Game.Update is for game-wide decisions. World.Update drives components on actors. The rotator is called there automatically; we do not add a rotator call to this loop. Rendering follows gameplay updates.

Briefly point to delta. Time-based behavior uses elapsed time rather than assuming a fixed frame rate. The runtime caps delta at 0.25 seconds.

**Takeaway:** know where gameplay runs, then leave frame-loop coordination to the application.

**Transition:** “Now let's look at how our rotator gets into that World.”

## 2. Find the actor and attach behavior — 3 minutes

**Open:** Game.cpp → Game::ConfigureWorld.

**Show:** the block with FindActor("OrientationGizmos_TGE"), `AddComponent<SpinComponent>("WalkthroughRotator")`, and SetDegreesPerSecond(45.0f).

**Explain in order:**

1. FindActor uses the exported actor instance name. The if statement handles an absent actor.
2. The template argument chooses the C++ component class.
3. WalkthroughRotator names this instance. Its name must be nonempty and unique on its actor.
4. The setter configures speed before BeginPlay.

**Explain where new code belongs:** game-specific components go under Source/Application/Game, beside SpinComponent or in a feature subfolder. ConfigureWorld handles per-scene attachment and configuration. Game.Initialize and Shutdown handle run-wide setup and cleanup; Game.Update handles game-wide decisions.

### Show the alternative: spawning an actor

Point to an existing SpawnActor call further down ConfigureWorld. The existing skeletal-demo blocks show engine mesh and animation component attachment. Use them as API examples, not as a player implementation added by this walkthrough.

**Explain:** use an imported actor or create one in code. A new actor has a transform but needs a visual component to become visible. World currently suffixes duplicate actor names; inspect the returned actor's actual name if you need later lookup.

**Takeaway:** finding or spawning the actor, adding behavior, and configuring it are separate steps.

## 3. Open the custom class — 2 minutes

**Open:** SpinComponent.h.

**Show:** inheritance from Component, setters, lifecycle overrides, and private state.

**Explain:**

- Component adds behavior to an actor. Actor is final here; this example extends gameplay through composition.
- Setters provide configuration. Speed defaults to 25 degrees per second; our setup overrides it to 45.
- BeginPlay initializes runtime state; Update advances behavior.
- The component owns its yaw state instead of placing it in the frame loop.
- Target selection is optional; our demo targets the actor.

**Briefly open:** [SceneComponent.h](../Source/Engine/GameFramework/Components/SceneComponent.h).

**Explain:** SceneComponent adds a transform relative to the actor. Use it when the new component needs its own spatial offset. SpinComponent changes an existing transform, so it only needs Component.

**Build reminder:** [Game's Premake file](../Source/Application/Game/premake5.lua) includes headers and C++ files recursively. Regenerate the Visual Studio project or add new files to the current Game project according to the team's workflow. A file on disk is not automatically part of an already generated project.

## 4. Show the engine APIs they will use — 2 minutes

**Open:** Actor.h. Point to these APIs without explaining every implementation detail:

| API | Explain |
| --- | --- |
| `AddComponent<T>` | Construct and attach a component; the actor owns it |
| `GetComponent<T>` | Retrieve the first matching C++ type; check for null |
| FindComponent(name) | Retrieve one instance when several components can share a type |
| GetTransform() | Read or change the actor's transform |

**Explain:** type tells us what a component does; instance name tells us which particular component we mean. For named lookup, check or cast to the expected type before using it.

**Open:** Component.h. Show GetOwner, GetWorld, lifecycle methods, and SetEnabled.

**Explain:** custom components use these APIs to reach their actor and World. Owner is assigned during attachment, so use it after attachment or during lifecycle methods, not in the constructor.

**Show the folder:** [GameFramework/Components](../Source/Engine/GameFramework/Components/). Point to StaticMeshComponent, SkeletalMeshComponent, AnimatorComponent, CameraComponent, and lights. Include and reuse existing engine components rather than duplicating their functionality.

**Ownership rule:** World owns actors; actors own components. Returned pointers are borrowed. Do not delete them or retain them across destruction or scene replacement.

## 5. Follow the rotation implementation — 3 minutes

**Open:** SpinComponent.cpp. Visit these functions in order.

### SetDegreesPerSecond

**Show:** validation and assignment.

**Explain:** zero stops rotation, negative speed reverses it, and non-finite values are rejected. Code today and imported configuration later can use the same setter and behavior.

### BeginPlay

**Show:** finding the target and reading its authored rotation into myYaw.

**Explain:** initialization starts from the existing orientation. Other features can resolve dependencies here after initial construction and configuration.

### FindTargetTransform

**Show:** the actor-transform branch, then the named SceneComponent lookup.

**Explain:** no target name means rotate the owner. A name means find a SceneComponent and change its actor-relative transform. Missing or wrong-type targets return null. Keep the demo on the actor target.

### Update

**Show:** speed multiplied by deltaTime, the modulo, and the transform write.

**Explain:** 45 degrees per second over 0.1 seconds advances yaw by 4.5 degrees. A fixed angle per frame would make speed depend on frame rate. The modulo keeps the angle bounded.

Point to the existing pitch and roll passed back to SetLocalRotationDegrees. Those values are preserved; position and scale are unchanged.

**Briefly open:** [Transform.h](../Source/Engine/GameFramework/World/Transform.h) and its rotation comment. The vector stores **yaw, pitch, roll**: x is yaw about Y, y is pitch about X, and z is roll about Z. Vector x does not mean rotation about the X axis here.

**Optional demo:** change speed in ConfigureWorld, rebuild, and run. F4 reloads a scene; it does not compile changed C++ code.

## 6. Show who calls the component — 1 minute

**Open:** World.cpp → BeginPlay, then Update.

**Show:** component->BeginPlay() and the enabled/active checks around component->Update(deltaTime).

**Explain:** this connects back to the engine loop. World drives initialization and updates. Enabled components on active actors receive normal updates. Disabled components can still begin play.

World freezes its frame list before callbacks. Components added from those callbacks are picked up on a later World update. Components added by Game.Update before World.Update can be included that frame. Avoid relying on accidental ordering for interdependent behavior.

**Cleanup note:** a component that subscribes to input or runtime callbacks should unsubscribe in EndPlay. The rotator has no subscription, so it does not need an override. Do not manually add lifecycle calls to the main loop.

Skip the unrelated animation-demo controls below the component loop.

## 7. Show where building happens — 2 minutes

**Open:** GameApplication.cpp → ProcessPendingSceneLoad.

**Show:** import, BuildWorldFromSceneData, aGame.ConfigureWorld(*candidateWorld), replacement of myWorld, and myWorld->BeginPlay().

Explain the sequence while pointing to the calls:

    Exported file → importer → SceneData
        → builder creates the candidate World
        → Game.ConfigureWorld attaches and configures game behavior
        → application replaces the old World
        → camera fallback if needed → World.BeginPlay

**Briefly open:** WorldFromSceneData.cpp → BuildWorldFromSceneData and CreateComponent. Show actor spawning and one built-in mapping, such as StaticMeshData to StaticMeshComponent. Skip the material pipeline.

**Explain:** importer converts data, builder creates engine objects, Game supplies project behavior. That is why our attachment belongs in ConfigureWorld today.

**Demo:** F4 reload, then return to Game.cpp. A fresh World is configured again. Old pointers cannot be reused. A later load failing before replacement leaves the running World intact; initial loading must succeed.

Current RequestSceneLoad takes a filesystem path. Older guides mentioning SceneId do not describe the current signature. Gameplay requests changes through GameApplication rather than clearing or replacing the World itself.

## 8. Connect it to the team's features — 1 minute

Stay in the source folders rather than starting another architecture tour.

| Feature | Explain the integration |
| --- | --- |
| Player | Components receive intent, move the actor, and use mesh/animation components |
| Enemy | Actor-specific behavior connects decisions to movement and animation |
| Navmesh | Pathfinding operates on navigation data; a component requests and follows paths |
| VFX | A component connects effects to transforms and lifecycle; the feature supplies simulation and renderer integration |

These are design examples, not features implemented by this demo. Not every algorithm needs to inherit Component. A component connects a feature to the runtime.

If input comes up, show the named ReloadScene listener in Game.Initialize and its removal in Shutdown. For actor-specific input, the equivalent is subscription in BeginPlay and removal in EndPlay.

## 9. Explain the Unreal/factory TODO — 1 minute today

**Open:** [the Blockout export](../Content/ExportedScenes/lvl_blockout/Lvl_Blockout_Level.json), UnrealSceneImporter.cpp, and SceneData.h.

**Show:** one built-in exported component, the UnrealComponentType::Custom case, and the placeholder path.

**Explain what is known:** records have an instance Name, TypeID, Parent, Tags, and Transform, plus built-in type fields. Blockout has no custom example. The Custom category cannot distinguish a rotator from a player-movement class. Custom records currently become placeholders.

**Explain what remains:** export an actual custom component at school and inspect its class identity and settings. Preserve that data through the importer and SceneData, add the Game-owned registry, and connect it to world construction. Do not present guessed JSON fields as an established contract.

The intended registry is small: explicit type-to-creator registrations, owned by Game, with a separate registration file and useful configuration errors. RootIssue is a reference; we do not need its singleton or automatic registration.

Return to the TODO above the manual attachment in Game.ConfigureWorld. The factory replaces that step. Remove the manual version once imported creation works to avoid duplicates.

**Tomorrow's additions:**

1. Open actual Unreal settings and the real exported record.
2. Follow the data through the importer and SceneData.
3. Open the completed registry and rotator registration.
4. Change speed in Unreal, export, load, and show the result.

See [the implementation plan](GameFrameworkWalkthroughPlan.md) for deferred work.

## Close with the files they need

Return to **Game.cpp → SpinComponent.h → SpinComponent.cpp**.

The repeatable process is: create the class in Game, find or spawn its actor, reuse engine components, attach and configure behavior, implement lifecycle, and verify behavior plus scene reload.

Invite questions about where their own feature connects. Use the example and actual APIs to answer.

## Quick reference for questions

| Symptom | First place to look |
| --- | --- |
| Actor lookup fails | Exported name and World.FindActor |
| No component updates | Attachment, enabled state, active actor, World.Update |
| Null dependency | Expected type/name and lookup timing |
| Unexpected orientation | Actor versus component target, yaw/pitch/roll order, competing writers |
| Duplicate component error | Repeated attachment of the same instance name |
| Reload crashes | Retained pointers or subscriptions to destroyed objects |
| Import error | Actual exported fields and importer diagnostics before gameplay code |

The Blockout export was missing the material TwoSided field. The importer now defaults an absent field to false and validates a present one. Mention this only if helpful during questions: it illustrates diagnosing the right layer.
