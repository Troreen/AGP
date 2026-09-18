#include "GameFramework/Runtime/GameContext.h"

GameContext::GameContext() : myWorld(std::make_unique<World>(&myInput))
{
}

bool GameContext::LoadScene(std::string name)
{
	if (!myAcceptSceneRequests || name.empty())
	{
		return false;
	}
	myPendingScene = std::move(name);
	return true;
}

bool GameContext::ReloadScene()
{
	return LoadScene(mySceneName);
}
