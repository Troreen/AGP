#pragma once
#include "MeshLibrary.h"
#include "GameComponentRegistry.h"
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
	CustomComponent* CreateCustomComponent(Actor& anActor, const CustomComponentData& aCustomComponentData) const;

private:
	// Keeps the FBX importer initialized while AssetRegistry loads FBX assets.
	MeshLibrary myMeshLibrary;
	std::vector<unsigned> myInputListenerIDs;
	GameComponentRegistry myComponentRegistry;
};
