#include "GameFramework/Runtime/GameContext.h"

GameContext::GameContext() : myWorld(std::make_unique<World>(&myInput))
{
}

bool GameContext::LoadScene(const SceneType& aScene)
{
	if (!myAcceptSceneRequests || aScene == SceneType::None)
	{
		return false;
	}
	myPendingScene = aScene;
	return true;
}

bool GameContext::ReloadScene()
{
	return LoadScene(mySceneName);
}
