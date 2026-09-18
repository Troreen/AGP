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

	virtual void ConfigureWorld(World&)
	{
	}

	virtual void Initialize(GameContext&) = 0;

	virtual void Update(GameContext&, float)
	{
	}

	virtual void OnSceneLoaded(GameContext&, const std::string&)
	{
	}

	virtual void OnSceneLoadFailed(GameContext&, const std::string&, const std::string&)
	{
	}

	// Called even when Initialize partially fails. Component EndPlay follows this.
	virtual void Shutdown(GameContext&)
	{
	}
};
