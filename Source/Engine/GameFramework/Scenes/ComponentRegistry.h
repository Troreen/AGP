#pragma once
#include "GameFramework/Scenes/SceneData.h"
#include "GameFramework/World/World.h"

// Builds the closed set of engine components described by SceneData.
class ComponentRegistry
{
public:
	std::unique_ptr<World> CreateWorld(const SceneData& scene, IAssetResolver& assets, InputSystem* input = nullptr,
	                                   CommonUtilities::Vector2u size = {1280, 720}) const;
};
