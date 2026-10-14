#include "BT_Condition_IsPlayerInLOS.h"
#include <GameFramework/ServiceLocator.h>
#include <GameFramework/World/Actor.h>
#include <GameFramework/World/Transform.h>

BT_Condition_IsPlayerInLOS::BT_Condition_IsPlayerInLOS(BehaviourTree* aBehaviourTree)
	: BT_LeafNodeCondition(aBehaviourTree)
{
	myLOSRadius = 500;
}

bool BT_Condition_IsPlayerInLOS::CheckCondition() const
{
	Actor* player = ServiceLocator::GetInstance().GetPlayer();

	if (player == nullptr)
	{
		return false;
	}

	CommonUtilities::Vector3f playerPos = player->GetTransform().GetWorldPosition();
	CommonUtilities::Vector3f enemyPos = myBlackBoard->GetActor(BlackBoardVariable_Actors::OwnerActor)->GetTransform().GetWorldPosition();

	if ((playerPos - enemyPos).Length() < myLOSRadius)
	{
		return true;
	}
    return false;
}
