#pragma once
#include "GameFramework/World.h"
#include "WorldAccess.h"
#include "GameFramework/Integration/ISceneSource.h"
#include "GameFramework/Registration/ComponentRegistry.h"
#include "GameFramework/GameTime.h"
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
        std::unique_ptr<World> myWorld = WorldAccess::Create(&myInput);
        ComponentRegistry myRegistry;
        std::filesystem::path myContentRoot;
        CommonUtilities::Vector2u myClientSize;
        std::atomic<bool> myQuitRequested = false;
        bool myClosing = false;
    };
}
