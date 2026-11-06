#pragma once

#include "../AssetHandling/SceneAsset.h"
#include "../Components/CustomComponentRegistry.h"
#include "../World/World.h"
#include "../Helper/Event.hpp"
#include "SceneWorldBuilder.h"

struct AmbientSettings;

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	void Initialize(const std::filesystem::path& aContentRoot);

	bool LoadScene(const std::shared_ptr<SceneAsset>& aScene);
	bool ReloadCurrentScene();
	bool ProcessSceneLoadRequest();

	void Cleanup();

	Event<>& OnLoadScene() { return myLoadSceneCallback; }

	bool HasRequestedScene() const { return myRequestedScene != nullptr; }

	CustomComponentRegistry& GetCustomComponentRegistry() { return myCustomComponentRegistry; }
	World* GetLoadedWorld() { return myCurrentWorld.get(); }
	const std::shared_ptr<SceneAsset>& GetCurrentScene() { return myCurrentScene; }

private:
	void TryLoadAmbientSettings(const std::filesystem::path& aSettingsFile, AmbientSettings& inoutAmbientSettings);

	SceneFallbackAssets myFallbackAssets;
	CustomComponentRegistry myCustomComponentRegistry;

	std::unique_ptr<World> myCurrentWorld;

	std::shared_ptr<SceneAsset> myRequestedScene;
	std::shared_ptr<SceneAsset> myCurrentScene;

	Event<> myLoadSceneCallback;

	bool myIsDisabled;
};
