#pragma once
#include <BehaviourTree/BaseNodes/BT_LeafNodeAction.h>
#include <BehaviourTree/BlackBoard.h>

class BT_Action_Idle : public BT_LeafNodeAction
{
public:
	BT_Action_Idle() = delete;
	BT_Action_Idle(BlackBoard* aBlackBoard) : BT_LeafNodeAction(aBlackBoard) {}

	void OnEnter() override;

	BT_NodeStatus Update() override;
private:
};
