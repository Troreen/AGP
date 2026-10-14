#include "BT_Action_FollowPlayer.h"
#include <GameFramework/ServiceLocator.h>
#include <GameFramework/World/Actor.h>
#include <Application/Game/EnemyComponent.h>

BT_Action_FollowPlayer::BT_Action_FollowPlayer(BlackBoard* aBlackBoard) : BT_LeafNodeAction(aBlackBoard)
{

}

BT_NodeStatus BT_Action_FollowPlayer::Update()
{
	Actor* player = ServiceLocator::GetInstance().GetPlayer();

	if (player == nullptr)
	{
		return BT_NodeStatus::Failure;
	}

	Actor* enemy = myBlackBoard->GetActor(BlackBoardVariable_Actors(BlackBoardVariable_Actors::OwnerActor));

	CommonUtilities::Vector3f playerPos = player->GetTransform().GetWorldPosition();
	CommonUtilities::Vector3f enemyPos = enemy->GetTransform().GetWorldPosition();
	CommonUtilities::Vector3f direction = (playerPos - enemyPos).GetNormalized();

	if ((playerPos - enemyPos).Length() > 500)
	{
		return BT_NodeStatus::Failure;
	}
	else if ((playerPos - enemyPos).Length() < 10)
	{
		return BT_NodeStatus::Success;
	}
	else
	{
		enemy->GetTransform().SetWorldPosition(enemyPos + direction * 50.f * ServiceLocator::GetInstance().GetDeltaTime());
		enemy->GetComponent<EnemyComponent>()->SetActionNode(this);

		return BT_NodeStatus::Running;
	}
}
