#include "BT_LeafNodeCondition.h"
#include <BehaviourTree/BehaviourTree.h>
#include <BehaviourTree/BlackBoard.h>

BT_LeafNodeCondition::BT_LeafNodeCondition(BehaviourTree* aBehaviourTree)
{
	myBehaviourTree = aBehaviourTree;
	myBlackBoard = &myBehaviourTree->GetBlackBoard();
}

BT_NodeStatus BT_LeafNodeCondition::Update()
{
	bool checkCondition = CheckCondition();

	myBehaviourTree->AddLeafNodeCondition(this, checkCondition);

	if (checkCondition)
	{
		return BT_NodeStatus::Success;
	}
	else
	{
		return BT_NodeStatus::Failure;
	}
}
