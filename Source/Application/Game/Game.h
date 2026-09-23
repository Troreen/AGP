#pragma once
#include "GameFramework/Runtime/IGame.h"
#include "InputMapper.h"
#include <vector>

// The game entry object. For another project, replace this class and its content
// while reusing GameFramework. It owns game-session setup, not the engine loop.
// Read in this order: Main.cpp -> Game.cpp -> GameScene.cpp.
class Game final : public IGame
{
public:
	Game();
	~Game() override;
	void ConfigureWorld(World& world) override;
	void Initialize(GameContext& context) override;
	void Update(GameContext& context, float deltaTime) override;
	void Shutdown(GameContext& context) override;
private:
	std::vector<unsigned> myInputListenerIDs;
	bool myToggleChestRequested = false;
	bool myAttachChildRequested = false;
};
