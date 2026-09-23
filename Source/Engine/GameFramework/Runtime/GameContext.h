#pragma once
#include "GameFramework/World/World.h"
#include "Vector2.hpp"
#include <filesystem>
#include <optional>
#include "GameFramework/Scenes/SceneData.h"

// Owns session world state. Game callbacks run on the application thread.
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


	// True accepts a request into one last-write-wins slot; it does not load the scene.
	// Requests are applied after Initialize or at a frame boundary, never during Update.
	// Failed construction retains the slot for retry. Successful scene activation clears
	// it, including requests made by that scene's BeginPlay or OnSceneLoaded callbacks.
	bool LoadScene(const SceneType& aScene);
	// Requires a current scene type assigned by a successful scene replacement.
	bool ReloadScene();

	// Valid after a scene replacement has assigned the current type.
	const SceneType& GetSceneType() const
	{
		return myCurrentSceneType;
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
	std::unique_ptr<World> myWorld;
	std::optional<SceneType> myPendingScene;
	SceneType myCurrentSceneType;
	std::filesystem::path myContentRoot;
	CommonUtilities::Vector2u myClientSize;
	bool myQuitRequested = false;
	bool myAcceptSceneRequests = true;
	friend class GameApplication;
};
