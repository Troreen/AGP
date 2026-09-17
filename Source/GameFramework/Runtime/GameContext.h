#pragma once
#include "GameFramework/World/World.h"
#include <filesystem>
#include <optional>

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

	const GameInput& GetInput() const
	{
		return myInput;
	}

	// Requests are applied at the next frame boundary, never in the middle of Update.
	bool LoadScene(std::string name);
	bool ReloadScene();

	const std::string& GetSceneName() const
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
	GameInput myInput;
	std::unique_ptr<World> myWorld;
	std::optional<std::string> myPendingScene;
	std::string mySceneName;
	std::filesystem::path myContentRoot;
	CommonUtilities::Vector2u myClientSize;
	bool myQuitRequested = false;
	bool myAcceptSceneRequests = true;
	friend class GameApplication;
};
