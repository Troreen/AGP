#include "GameContext.h"
#include "Internal/SessionState.h"
#include "Internal/WorldAccess.h"

#include "Internal/SceneServiceAccess.h"
GameContext::GameContext() : myState(std::make_unique<GameFrameworkInternal::SessionState>())
{
    GameFrameworkInternal::WorldAccess::BindServices(*myState->myWorld, &myState->myScenes, myState->myAssets.get(), &myState->myTime);
    GameFrameworkInternal::SceneServiceAccess::Get(myState->myScenes).BoundWorld = myState->myWorld.get();
}
GameContext::~GameContext() { GameFrameworkInternal::WorldAccess::Shutdown(*myState->myWorld); }
World& GameContext::GetWorld() { return *myState->myWorld; }
const GameInput& GameContext::GetInput() const { return myState->myInput; }
SceneService& GameContext::GetScenes() { return myState->myScenes; }
const AssetLookup& GameContext::GetAssets() const { return *myState->myAssets; }
const GameTime& GameContext::GetTime() const { return myState->myTime; }
const std::filesystem::path& GameContext::GetContentRoot() const { return myState->myContentRoot; }
CommonUtilities::Vector2u GameContext::GetClientSize() const { return myState->myClientSize; }
void GameContext::SetActiveCamera(CameraComponent* camera) { myState->myWorld->SetActiveCamera(camera); }
void GameContext::RequestQuit()
{
    GameFrameworkInternal::WorldAccess::EnsureMutationAllowed(*myState->myWorld);
    myState->myQuitRequested = true;
}
