#include "BT_Action_Idle.h"
#include <Application/Game/BehaviourTree/BehaviourTreeComponent.h>
#include <GameFramework/World/Actor.h>
#include <GameFramework/Components/AnimatorComponent.h>
#include <BehaviourTree/BlackBoard.h>

void BT_Action_Idle::OnEnter()
{
	reinterpret_cast<AnimatorComponent*>(myBlackBoard->GetComponent(BlackBoardVariable_Components::OwnerAnimation))->PlayAnimation("Idle", true);
}

BT_NodeStatus BT_Action_Idle::Update()
{
	Actor* enemy = myBlackBoard->GetActor(BlackBoardVariable_Actors(BlackBoardVariable_Actors::OwnerActor));
	enemy->GetComponent<BehaviourTreeComponent>()->SetActionNode(this);

	return BT_NodeStatus::Running;
}
