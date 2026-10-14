# Custom component initialization plan

Status: Implemented as `InitializeCustomVariables`. Broader lifecycle changes remain deferred.

`CustomComponent::InitializeCustomVariables` configures a newly attached custom component from imported variables before `BeginPlay`.

## Current interface

```cpp
virtual void InitializeCustomVariables(const CustomComponentVariables& someVariables) = 0;
```

Use this hook for configuration, validation, and internal setup. Keep setup that depends on other components in `BeginPlay()`.

## Lifecycle order

The scene builder currently performs these steps:

1. Create and attach the custom component.
2. Apply common data, including tags and enabled state.
3. Call `InitializeCustomVariables(variables)`.
4. Call `BeginPlay()` later, after component creation has finished.

Initialization runs while components are still being created. Later components may not exist yet, so this hook cannot assume all component references are available.

## Implementation steps

- Rename the declaration in `Source/Engine/GameFramework/Components/CustomComponent.h` and document the setup contract.
- Rename the override and definition in `Source/Application/Game/Components/SpinComponent.h` and `.cpp`.
- Update the call and comment in `Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp`.
- Update `Tests/GameFramework/CustomComponentTests.cpp`, including lifecycle terminology where appropriate.
- Search for remaining references to the old name and update any relevant documentation.

## Decision before implementation

The current hook belongs to scene-builder setup. `Actor::AddComponent()` creates and attaches components without calling `InitializeCustomVariables`. A future universal initialization hook needs a defined call path and timing for runtime creation, including how variables are supplied and how repeated initialization is prevented.

The initial rename can preserve the current call order. Broader lifecycle behavior should be designed explicitly if needed.

## Verification

Build the affected targets and run the existing custom component tests. Preserve their checks that setup receives an owner and common data, runs once before `BeginPlay`, and leaves reference resolution until all components exist. Confirm imported `SpinSpeed` still drives rotation.
