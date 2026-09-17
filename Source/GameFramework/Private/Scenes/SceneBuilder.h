#pragma once
#include "GameFramework/Integration/SceneData.h"
#include "GameFramework/Registration/ComponentRegistry.h"
#include "GameFramework/World.h"
#include "GameFramework/Components/CameraComponent.h"
#include "Vector2.hpp"

class SceneService;
class GameTime;
struct SceneBuildServices
{
    const GameInput* Input = nullptr;
    const AssetLookup* Assets = nullptr;
    SceneService* Scenes = nullptr;
    const GameTime* Time = nullptr;
    CommonUtilities::Vector2u ClientSize{1280, 720};
};
struct SceneBuildResult
{
    std::unique_ptr<World> Candidate;
    ComponentRef<CameraComponent> Camera;
    SceneDiagnostics Diagnostics;
    explicit operator bool() const { return Candidate != nullptr && Diagnostics.empty(); }
};
class SceneBuilder
{
public:
    // Allocates/configures/resolves the complete owned data set. No gameplay begins
    // here; failures release the candidate and retain source-addressed diagnostics.
    static SceneBuildResult Build(const SceneData& scene, const ComponentRegistry& registry,
        const SceneBuildServices& services = {});
};
