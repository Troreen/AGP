#pragma once
#include "GameFramework/World/World.h"
#include "GameFramework/Integration/GameFramework/Integration/LegacySceneBridge.h"
#include <atomic>
#include <filesystem>
#include <mutex>

namespace GameFrameworkInternal
{
    struct SessionState
    {
        GameInput myInput;
        std::unique_ptr<World> myWorld = std::make_unique<World>(&myInput);
        ComponentRegistry myRegistry;
        std::filesystem::path myContentRoot;
        CommonUtilities::Vector2u myClientSize;
        std::mutex mySceneMutex;
        GameFrameworkIntegration::LegacySceneBridge::Factory mySceneFactory;
        std::atomic<bool> mySceneRequested = false;
        std::atomic<bool> myQuitRequested = false;
        bool myClosing = false;
    };
}
