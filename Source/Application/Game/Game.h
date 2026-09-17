#pragma once
#include "GameFramework/Runtime/IGame.h"

// The game entry object. For another project, replace this class and its content
// while reusing GameFramework. It owns game-session setup, not the engine loop.
// Read in this order: Main.cpp -> Game.cpp -> GameScene.cpp ->
// GameComponents.cpp.
class Game final : public IGame
{
public:
	Game();
	~Game() override;
	void RegisterComponents(ComponentRegistry& registry) override;
	void Initialize(GameContext& context) override;
	void Update(GameContext& context, float deltaTime) override;
	void Shutdown(GameContext& context) override;
};
