#pragma once
#include "../../../Scenes/SceneBuilder.h"
#include <functional>
class GameContext;

namespace GameFrameworkIntegration
{
    // Migration only: ModelViewer and host regression fixtures use this bridge in
    // M1/M2. Removed in M3 when owned SceneData and SceneService replace recipes.
    class LegacySceneBridge
    {
    public:
        using Factory = std::function<SceneBuildResult(const ComponentRegistry&, const GameInput*)>;
        static void Request(GameContext& context, Factory factory);
    };
}
