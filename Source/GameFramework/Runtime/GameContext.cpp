#include "GameContext.h"
#include "Internal/SessionState.h"
#include "Internal/WorldAccess.h"

GameContext::GameContext() : myState(std::make_unique<GameFrameworkInternal::SessionState>()) {}
GameContext::~GameContext() { GameFrameworkInternal::WorldAccess::Shutdown(*myState->myWorld); }
World& GameContext::GetWorld() { return *myState->myWorld; }
const GameInput& GameContext::GetInput() const { return myState->myInput; }
const std::filesystem::path& GameContext::GetContentRoot() const { return myState->myContentRoot; }
CommonUtilities::Vector2u GameContext::GetClientSize() const { return myState->myClientSize; }
void GameContext::SetActiveCamera(CameraComponent* camera) { myState->myWorld->SetActiveCamera(camera); }
void GameContext::RequestQuit() { myState->myQuitRequested = true; }

void GameFrameworkIntegration::LegacySceneBridge::Request(GameContext& context, Factory factory)
{
    auto& session = *context.myState;
    if (session.myClosing) return;
    std::scoped_lock lock(session.mySceneMutex);
    session.mySceneFactory = std::move(factory);
    session.mySceneRequested = true;
}
