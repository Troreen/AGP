#pragma once
#include "SceneDescription.h"
#include "ComponentRegistry.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/CameraComponent.h"

struct SceneBuildResult
{
    std::unique_ptr<World> Candidate;
    ComponentHandle<CameraComponent> Camera;
    SceneDiagnostics Diagnostics;
    explicit operator bool() const { return Candidate != nullptr && Diagnostics.empty(); }
};
class SceneBuilder
{
public:
    // Never starts gameplay or modifies an existing world. Failures own diagnostics
    // and release the candidate before returning to the host's assertion boundary.
    static SceneBuildResult Build(const SceneDescription& scene, const ComponentRegistry& registry, const GameInput* input = nullptr);
};
