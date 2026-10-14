#include "BlackBoard.h"
#include <GameFramework/World/Actor.h>

void BlackBoard::SetActor(BlackBoardVariable_Actors aActorVariable, Actor* aValue)
{
	myActors[aActorVariable] = aValue;
}

Actor* BlackBoard::GetActor(BlackBoardVariable_Actors aActorVariable)
{
	if (myActors.find(aActorVariable) == myActors.end())
	{
		myActors[aActorVariable] = nullptr;
	}

	return myActors[aActorVariable];
}
