#pragma once
#include "GameFramework/World/World.h"
#include "GameFramework/Integration/GameFramework/Integration/ISceneSource.h"
#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Runtime/GameTime.h"
#include <atomic>
#include <filesystem>
#include <mutex>

namespace GameFrameworkInternal
{
    struct SessionState
    {
        GameInput myInput;
        GameTime myTime;
        SceneService myScenes;
        std::unique_ptr<GameFrameworkIntegration::ISceneSource> mySource;
        std::unique_ptr<GameFrameworkIntegration::AssetBindings> myAssets = std::make_unique<GameFrameworkIntegration::AssetBindings>();
        std::unique_ptr<World> myWorld = std::make_unique<World>(&myInput);
        ComponentRegistry myRegistry;
        std::filesystem::path myContentRoot;
        CommonUtilities::Vector2u myClientSize;
        std::atomic<bool> myQuitRequested = false;
        bool myClosing = false;
    };
}
