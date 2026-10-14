#include "BehaviourTree.h"

BehaviourTree::BehaviourTree()
{
	myShouldTick = true;
	myRoot = nullptr;
}

void BehaviourTree::Run()
{
	CheckCurrentLeafNodeConditions();

	if (myShouldTick)
	{
		myCurrentLeafNodeConditionAndResultsAtLastTick.clear();
		myRoot->Tick();
		myShouldTick = false;
	}
}

void BehaviourTree::Tick()
{
	myShouldTick = true;
}

void BehaviourTree::CheckCurrentLeafNodeConditions()
{
	for (LeafNodeConditionAndResultAtLastTick& leafNode : myCurrentLeafNodeConditionAndResultsAtLastTick)
	{
		if (leafNode.ResultAtLastTick != leafNode.LeafNodeCondition->CheckCondition())
		{
			myShouldTick = true;
			break;
		}
	}
}
