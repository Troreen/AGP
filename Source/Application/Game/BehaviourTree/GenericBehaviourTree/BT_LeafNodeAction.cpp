#include "BT_LeafNodeAction.h"
#include "BehaviourTree.h"
#include "..\BehaviourTreeComponent.h"

BT_LeafNodeAction::BT_LeafNodeAction(BehaviourTree* aTree)
	: BT_Node(aTree)
{
	myEnter  = [](){};
	myStart  = [](){};
	myAction = [](){};
	myEnd    = [](){};
}

void BT_LeafNodeAction::SetEnter(std::function<void()>&& aEnter)
{
	myEnter = aEnter;
}
void BT_LeafNodeAction::SetCondition(std::function<BT_NodeStatus()>&& aCondition)
{
	myCondition = aCondition;
}
void BT_LeafNodeAction::SetStart(std::function<void()>&& aStart)
{
	myStart = aStart;
}

void BT_LeafNodeAction::SetAction(std::function<void()>&& aAction)
{
	myAction = aAction;
}

void BT_LeafNodeAction::SetEnd(std::function<void()>&& aEnd)
{
	myEnd = aEnd;
}

void BT_LeafNodeAction::Exit()
{
	myEnd();
}

BT_NodeStatus BT_LeafNodeAction::Execute()
{
	myEnter();

	BT_NodeStatus condition = myCondition();

	if (condition == BT_NodeStatus::Running)
	{
		myStart();
	}

	return condition;
}

void BT_LeafNodeAction::Update()
{
	if (myCondition() != BT_NodeStatus::Running)
	{
		myTree->Tick();
		return;
	}

	myAction();
}
