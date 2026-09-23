#pragma once
#include <string>
class GameContext;
class ComponentRegistry;
class World;

// Run calls these on the application thread. Components are updated automatically.
class IGame
{
public:
	virtual ~IGame() = default;

	// Configures a candidate world before replacing the live world or calling BeginPlay.
	virtual void ConfigureWorld(World&)
	{
	}

	// Receives the session context after services/input setup, before initial world startup.
	virtual void Initialize(GameContext&) = 0;

	// Receives context and delta seconds after input dispatch, before world/audio updates.
	virtual void Update(GameContext&, float)
	{
	}

	// Receives context and the display scene name after the replacement world's BeginPlay.
	virtual void OnSceneLoaded(GameContext&, const std::string&)
	{
	}

	// Receives context, display scene name and construction/configuration error message.
	virtual void OnSceneLoadFailed(GameContext&, const std::string&, const std::string&)
	{
	}

	// Called on normal exit and protected startup/frame failures, even before Initialize.
	// Early window/content/graphics failures are outside this boundary. World cleanup
	// follows when reached; a normal-exit Shutdown exception skips explicit cleanup.
	virtual void Shutdown(GameContext&)
	{
	}
};
