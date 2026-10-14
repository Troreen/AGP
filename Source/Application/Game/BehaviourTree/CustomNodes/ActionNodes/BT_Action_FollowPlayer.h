#pragma once
#include <BehaviourTree/BaseNodes/BT_LeafNodeAction.h>
#include <BehaviourTree/BlackBoard.h>

class BT_Action_FollowPlayer : public BT_LeafNodeAction
{
public:
	BT_Action_FollowPlayer() = delete;
	BT_Action_FollowPlayer(BlackBoard* aBlackBoard);

	BT_NodeStatus Update() override;

private:
};
