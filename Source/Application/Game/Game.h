#pragma once
#include "MeshLibrary.h"
#include <vector>

class GameApplication;
class World;

class Game final
{
public:
	Game();
	~Game();
	void ConfigureWorld(World& aWorld);
	void Initialize(GameApplication& anApplication);
	void Update(World& aWorld, float aDeltaTime);
	void Shutdown();
private:
	MeshLibrary myMeshLibrary;
	std::vector<unsigned> myInputListenerIDs;
};
