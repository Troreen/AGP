#include "BehaviourTree.h"
#include "BT_Node.h"
#include "BT_LeafNodeCondition.h"
#include "BT_LeafNodeAction.h"

BehaviourTree::BehaviourTree()
{
	myTick = true;
}

void BehaviourTree::Run()
{
	CheckForChangeInCondition();

	if (myTick)
	{
		myConditionNodesToCheckForChange.clear();
		myTick = false;
		myRoot->Execute();
	}
}

BlackBoard& BehaviourTree::GetBlackBoard()
{
	return myBlackBoard;
}

void BehaviourTree::AddConditionNodeToCheck(BT_LeafNodeCondition* aConditionNode, bool aResult)
{
	myConditionNodesToCheckForChange.push_back({ aConditionNode, aResult });
}

void BehaviourTree::Tick()
{
	myTick = true;
}

void BehaviourTree::CheckForChangeInCondition()
{
	for (const ConditionNodeToCheckForChange& node : myConditionNodesToCheckForChange)
	{
		if (node.Condition != node.ConditionNodeToCheck->GetCondition())
		{
			myTick = true;
			return;
		}
	}
}
