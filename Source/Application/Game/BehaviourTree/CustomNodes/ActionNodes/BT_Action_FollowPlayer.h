#pragma once
#include <BehaviourTree/BaseNodes/BT_LeafNodeAction.h>
#include <BehaviourTree/BlackBoard.h>

class BT_Action_FollowPlayer : public BT_LeafNodeAction
{
public:
	BT_Action_FollowPlayer() = delete;
	BT_Action_FollowPlayer(BlackBoard* aBlackBoard);

	void OnEnter() override;

	BT_NodeStatus Update() override;
	void UpdateActorControllerData() const override;

private:
};
