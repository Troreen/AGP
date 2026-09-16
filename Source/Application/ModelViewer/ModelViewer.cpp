#include "ModelViewer.h"
#include "ModelViewerScene.h"
#include "GameFramework/Runtime/GameContext.h"
#include "Application.h"

ModelViewer::ModelViewer() = default;
ModelViewer::~ModelViewer() = default;

// Startup hook: compose the initial game world before gameplay and rendering run
// concurrently. A future JSON import will replace the builder behind this call;
// the application lifecycle and component behaviors can stay the same.
void ModelViewer::Initialize(GameContext& context)
{
	myScene = std::make_unique<ModelViewerScene>();
	myScene->Initialize(context);
	MVLOG(Log, "Game ready: RMB + WASD/Space/Ctrl camera, R pause chest spin, numpad 0-3 animation, 7-9 lights, Shift+7-9 place lights, Esc quit");
}

// Global session behavior belongs here. Per-actor behavior is attached as components
// and will run automatically after this hook; do not call component Update yourself.
void ModelViewer::Update(GameContext& context, float)
{
	// Session-level rules live here. Actor behavior is ticked by the world.
	if (context.GetInput().IsKeyPressed(Keys::ESCAPE)) context.RequestQuit();
}

// The host has joined gameplay work before entering this hook. Clear the selected
// camera and release our helper/cache; the host still owns and later destroys World.
// reset() is safe even if initialization failed before the helper was created.
void ModelViewer::Shutdown(GameContext& context)
{
	context.SetActiveCamera(nullptr);
	myScene.reset();
}
