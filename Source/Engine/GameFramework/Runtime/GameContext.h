#pragma once
#include "GameFramework/World/World.h"
#include "Vector2.hpp"
#include <filesystem>
#include <optional>
#include "GameFramework/Scenes/SceneData.h"

// Session state passed to game callbacks. Everything runs on the application thread.
class GameContext
{
public:
	GameContext();
	GameContext(const GameContext&) = delete;
	GameContext& operator=(const GameContext&) = delete;

	World& GetWorld()
	{
		return *myWorld;
	}

	InputSystem& GetInputSystem()
	{
		return myInput;
	}

	// Requests are applied at the next frame boundary, never in the middle of Update.
	bool LoadScene(const SceneType& aScene);
	bool ReloadScene();

	const SceneType& GetSceneType() const
	{
		return mySceneName;
	}

	const std::filesystem::path& GetContentRoot() const
	{
		return myContentRoot;
	}

	CommonUtilities::Vector2u GetClientSize() const
	{
		return myClientSize;
	}

	void RequestQuit()
	{
		myQuitRequested = true;
	}

private:
	InputSystem myInput;
	std::unique_ptr<World> myWorld;
	std::optional<SceneType> myPendingScene;
	SceneType mySceneName;
	std::filesystem::path myContentRoot;
	CommonUtilities::Vector2u myClientSize;
	bool myQuitRequested = false;
	bool myAcceptSceneRequests = true;
	friend class GameApplication;
};
