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

void BlackBoard::SetComponent(BlackBoardVariable_Components aComponentVariable, Component* aValue)
{
	myComponents[aComponentVariable] = aValue;
}

Component* BlackBoard::GetComponent(BlackBoardVariable_Components aComponentVariable)
{
	if (myComponents.find(aComponentVariable) == myComponents.end())
	{
		myComponents[aComponentVariable] = nullptr;
	}

	return myComponents[aComponentVariable];
}

