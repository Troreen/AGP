#pragma once
#include "GameFramework/Runtime/IGame.h"
#include <memory>

class ModelViewerScene;

// The game entry object. For another project, replace this class and its content
// while reusing GameFramework. It owns game-session setup, not the engine loop.
// Read in this order: Main.cpp -> ModelViewer.cpp -> ModelViewerScene.cpp ->
// ModelViewerComponents.cpp.
class ModelViewer final : public IGame
{
public:
	ModelViewer();
	~ModelViewer() override;
	void Initialize(GameContext& context) override;
	void Update(GameContext& context, float deltaTime) override;
	void Shutdown(GameContext& context) override;
private:
	// Owns the scene builder/asset caches. Actors themselves belong to context.World,
	// so destroying this helper is not the same as unloading a world or scene.
	std::unique_ptr<ModelViewerScene> myScene;
};
