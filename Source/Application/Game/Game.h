#pragma once

#include <vector>

class GameApplication;
class World;

class Game final
{
public:
	Game();
	~Game();
	void ConfigureWorld();
	void Initialize();
	void Update(World& aWorld, float aDeltaTime);
	void Shutdown();

private:
	std::vector<unsigned> myInputListenerIDs;
};
