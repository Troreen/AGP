# GameFramework integration walkthrough plan

Status: planning only. This document does not authorize implementation to start automatically. Resume the work when explicitly requested.

## Purpose

Prepare a presentation that teaches team members how to implement gameplay features using GameFramework. The audience can already import and run a scene but needs to understand where new C++ classes belong, how to attach and retrieve components, how scene construction works, and when gameplay code runs.

Use a small, configurable rotator as the running example. It demonstrates the integration process without implementing the player, enemies, navigation, or VFX features assigned to other teammates.

The intended final example is an actor authored in Unreal, exported with a custom component and rotation settings, then loaded by our game with a working C++ rotator component. The game-side work can be prepared here; the Unreal authoring and exporter work will be finished and verified at school.

## Deliverables

- A game-owned component creation extension and a configurable rotator component, after implementation is requested.
- A documented export contract and a small scene fixture demonstrating the agreed contract.
- An editable presentation in English, provisionally sized for a 20–30 minute session including code walkthrough and demonstration.
- A companion Markdown walkthrough with code locations, short examples, and instructions teammates can revisit.
- A clear account of what has been implemented and verified, and what still requires Unreal or the school environment.

## Scope and boundaries

The implementation should demonstrate one complete feature integration path. Keep it small enough that a teammate can understand and repeat it.

Game-specific behavior belongs in the Game application. GameFramework should expose the minimum extension needed to connect imported component data to game-owned creation logic without depending on game classes.

The importer reads source data. Runtime construction creates objects and assigns configuration. Component lifecycle methods connect dependencies, run behavior, and clean up resources.

Do not implement player control, combat, pathfinding, enemy AI, or a VFX system. Explain how those features could use the same integration pattern. Do not introduce a general reflection system, scripting engine, or automatic registration framework for this example.

Preserve unrelated work in the repository. At planning time, existing modifications were present in `Dependencies/SetupFmodSdk.ps1` and `Premake/EnsurePremake.ps1`.

## Current repository starting point

These observations describe this checkout and must be checked again before implementation:

- Custom Unreal component records currently preserve common metadata and become runtime placeholders.
- Scene data does not currently provide a general payload for custom gameplay settings.
- The world builder directly creates supported engine components.
- `Game::ConfigureWorld` is an existing place to add game behavior to a candidate World before it begins play.
- Actors expose component creation and lookup APIs. A gameplay feature can be composed from components rather than requiring a new Actor subclass.
- `SpinComponent` is an existing rotation example to inspect before deciding whether to reuse, adapt, or replace it for the walkthrough.
- Some documentation describes older APIs or intended behavior. Use current code as the authority and update affected documentation where necessary.

Primary code locations to review:

| Area | Location |
| --- | --- |
| Game configuration and behavior | `Source/Application/Game/Game.h` and `Game.cpp` |
| Runtime startup, scene loading, and frame loop | `Source/Application/Game/GameApplication.h` and `GameApplication.cpp` |
| Existing rotation example | `Source/Application/Game/SpinComponent.h` and `SpinComponent.cpp` |
| Actor, component, and transform APIs | `Source/Engine/GameFramework/World/` |
| Existing engine components | `Source/Engine/GameFramework/Components/` |
| Scene interchange data | `Source/Engine/GameFramework/Scenes/SceneData.h` |
| Runtime world construction | `Source/Engine/GameFramework/Scenes/WorldFromSceneData.h` and `.cpp` |
| Unreal source parsing and conversion | `Source/Engine/GameFramework/UnrealSceneImporter/` |
| Game project file inclusion | `Source/Application/Game/premake5.lua` |
| Existing framework and application checks | `Tests/GameFramework/` |

## Phase 1: agree on the feature and integration contract

Before changing code, settle the smallest useful rotator behavior and trace the current scene-loading path.

The proposed example rotates the owning actor at a configurable speed expressed in degrees per second. Preserve its authored position, scale, and unrelated rotation axes. Use delta time so the behavior does not depend on frame rate. Decide whether targeting an individual scene component is useful enough to include; rotating only the actor is the simpler teaching example.

Agree on how an Unreal component identifies its intended C++ type. Prefer a stable type identifier separate from the editable component instance name. If the exporter requires name-based identification, document the convention and its limitations explicitly.

Define the minimum fields needed: component type, instance name, enabled state, rotation speed, and any common metadata required by the existing pipeline. Establish defaults, accepted types, units, and useful diagnostics for invalid values or unsupported types.

Inspect a real export and the available exporter before claiming a JSON schema is established. A fixture created here is a proposed contract until the school exporter produces matching data.

Outcome: an agreed example and data contract that can be implemented without guessing how Unreal currently exports custom properties.

## Phase 2: design the game-owned creation extension

Choose a small extension point that lets the game create custom components while the engine continues creating its built-in components.

Evaluate passing a game-owned creation callback or factory into world construction. Compare that with a game configuration pass over preserved custom scene records if it better fits the existing lifecycle. Choose one approach rather than building both.

The selected approach should:

- Keep game component includes and creation rules in the Game application.
- Preserve custom identity and settings across the importer-to-scene-data boundary.
- Create and configure custom components before `BeginPlay`.
- Retain existing scene behavior when no custom factory is provided.
- Define whether unknown custom types remain placeholders or cause a diagnostic.
- Report invalid configuration with actor and component context.
- Fit the candidate-World construction path so a failed replacement does not damage the running scene.
- Make dependency lookup reliable even when imported components appear in a different order; resolve dependencies after construction where appropriate.

Outcome: a concrete design and list of affected files, ready for implementation review.

## Phase 3: implement the rotator example and game-side mapping

Start only after the user requests implementation.

Create or adapt a rotator component in the Game application. Give it a straightforward configuration API, a short lifecycle implementation, and comments that explain the integration responsibilities. Confirm new source files are included in the generated build projects.

Implement the game-owned mapping from the agreed custom type to that class. Add only the scene-data and importer support needed to carry the agreed settings. Keep game behavior out of the parser.

Provide a small fixture that follows the documented contract and can be used before the Unreal authoring work is available. Use an existing suitable asset where possible. Keep the demonstration repeatable and avoid adding rotators to unrelated production actors automatically.

Outcome: a working C++ integration path and a readable example suitable for showing on slides and in the editor.

## Phase 4: verify the game-side path

Use focused checks appropriate to the new integration boundary:

- A recognized custom record creates the expected runtime component with its settings.
- Defaults, disabled components, unknown types, and invalid settings follow the documented policy.
- The component is configured before it begins play and updates through the normal World lifecycle.
- Rotation advances according to elapsed time while preserving unrelated transform values.
- Existing built-in scene components still load correctly.
- Reloading the scene creates a fresh component without duplicate attachment or stale references.
- Failed custom construction follows the existing candidate-World failure behavior.

Build and run relevant tests where the environment supports them. State any unavailable SDK, asset, graphics, or runtime dependency precisely. Do not describe fixture-only verification as a verified Unreal export round trip.

Outcome: evidence of what works locally and an explicit list of checks remaining at school.

## Phase 5: finish and verify Unreal authoring at school

Use the documented contract to create an Unreal component or authoring equivalent that exposes the rotator settings.

Attach it to a visible actor, set a recognizable speed, export the scene, and inspect the actual output. Adapt the exporter or contract if needed so identity and values match the game-side reader.

Load that export in the game. Confirm the actor rotates, changing the authored speed changes runtime behavior, and disabling or removing the component behaves as expected. Test a scene reload.

Capture useful screenshots of the Unreal component settings and the running result. These should replace proposed or illustrative authoring content in the final deck.

Outcome: an end-to-end demonstration verified against a real Unreal export.

## Phase 6: prepare the presentation and companion walkthrough

Prepare a deck around the actual example, with short code snippets and speaker notes. Explain the integration steps in the order teammates will perform them. Keep the architecture overview brief and connect every concept to a concrete implementation task.

Proposed presentation sequence:

| Slide | Subject | Teaching purpose |
| --- | --- | --- |
| 1 | Implementing gameplay with GameFramework | State what teammates will know how to do |
| 2 | The rotator example | Show the actor, authored setting, and runtime result |
| 3 | Where your code belongs | Distinguish game components, independent systems, and existing engine components |
| 4 | Actors and existing components | Show actor lookup or creation, attachment, and component retrieval |
| 5 | The custom component class | Explain inheritance, configuration, ownership, and lifecycle |
| 6 | Rotation during Update | Show the small behavior implementation and delta time |
| 7 | Custom component data | Show identity, settings, defaults, and validation |
| 8 | The game-owned factory | Show the mapping from imported data to runtime class |
| 9 | Scene construction and BeginPlay | Explain when objects are created, configured, and activated |
| 10 | The frame loop | Briefly locate input, Game update, World update, audio, and rendering |
| 11 | Authoring and exporting in Unreal | Walk through attaching the component and changing its settings |
| 12 | Applying the pattern to team features | Connect it to player movement, enemies, navmesh, and VFX |
| 13 | Remaining gaps and implementation checklist | Explain what is still missing and how to start another feature |

Aim for approximately 20 minutes of explanation and demonstration, leaving up to 10 minutes for questions. Adjust slide count and pacing after rehearsal; the important outcome is that teammates can repeat the process.

The Markdown walkthrough should include the complete small snippets, repository-relative code links, the export contract, the ordered integration steps, and troubleshooting guidance. The deck should contain only the code needed to explain each step.

Until the school work is complete, explicitly label Unreal screenshots or procedures that still need to be supplied. Use clear status wording such as implemented, verified with a fixture, proposed, and awaiting Unreal verification.

## Phase 7: rehearse and finalize

Rehearse the deck alongside the code locations and working example. Check that filenames, snippets, factory behavior, and exported field names match the final implementation.

Keep a fallback demonstration using the verified fixture and captured runtime result in case live Unreal or game execution is unavailable. Inspect every slide for readability and code fit, and verify that the delivered PowerPoint remains editable.

Outcome: a presentation, walkthrough, and repeatable demonstration that accurately reflect the engine teammates will use.

## Decisions to resolve when work resumes

1. Actual custom-component export format and access to the exporter or representative export.
2. Stable type identifier versus an agreed name-based convention.
3. Factory callback during construction versus a game-owned configuration pass.
4. Reuse of `SpinComponent` versus a dedicated rotator class and game folder layout.
5. Actor-only rotation versus an optional target scene component.
6. Unknown-type handling, defaults, and invalid-property diagnostics.
7. Demonstration scene, available assets, and presentation visual style.

## Suggested next-session request

> Read Docs/GameFrameworkWalkthroughPlan.md and recheck the current repository. Start with the integration contract and factory design, using current code as the authority. When implementation is explicitly authorized, implement the smallest game-owned creation extension and configurable rotator, verify the fixture-based path, and document the remaining Unreal work. Then prepare the presentation and companion walkthrough. Keep unrelated changes intact and distinguish local verification from the real Unreal round trip.

This plan can be used as context in a later MCP-assisted session. Tool access does not itself resolve exporter availability or authorize new work; follow the user's instructions for that session.

## AI Pair iteration checkpoint

Implemented: configurable SpinComponent, direct attachment to imported OrientationGizmos_TGE in Game.ConfigureWorld, and Blockout as the current scene. The programmer confirmed that the game runs and the spinning works after making TwoSided optional for older scene exports.

TODO for school on 10 October 2026: inspect an actual custom component export before designing the registry payload and identity mapping. Defer the factory, importer extension, and Unreal section of the presentation. Provisional custom scene data and factory hooks were removed.

Preferred future design: Game-owned registry, explicit Register and Create operations, separate registration file, checked properties, and duplicate-registration errors. Take inspiration from RootIssue without copying its singleton.

Verified by the programmer: Cube2 spins and F4 reload does not crash. After changing the target to OrientationGizmos_TGE, the programmer also confirmed that it works in the running game.

Next: prepare the code walkthrough and presentation sections supported by the demo, then finish the export contract and factory at school.

## Presentation preparation checkpoint

Prepared [the companion walkthrough](GameFrameworkWalkthrough.md) and [the 15-slide PowerPoint presentation](Presentations/GameFrameworkWalkthrough.pptx), with editable text, speaker notes, and code references. [The slide content source](Presentations/GameFrameworkWalkthrough.slides.json) is also available for future revisions.

The deck focuses on code location, actors, component attachment and lookup, lifecycle, rotation, scene construction, frame order, and integration of independent systems. The Unreal export and registry section is explicitly marked TODO. Add real exporter data, completed factory code, and Unreal screenshots after school verification.

The deliverable is PPTX, as requested by the user. Present it in PowerPoint with Presenter View for speaker notes. The slide layouts were visually checked during preparation, and the PPTX package, slide count, text editability, and note contents were checked. PowerPoint itself is not installed in this environment, so rehearse the final PPTX in PowerPoint at school before presenting.

## Revised delivery format

The guided Markdown code tour is now the main presentation. GameFrameworkWalkthrough.md gives the files and functions to open, what to show, what to explain, and transitions. Include a brief engine-loop overview near the start, then focus on component implementation. The PowerPoint is an earlier supporting artifact. Unreal/exporter/factory stops remain deferred until school verification.
