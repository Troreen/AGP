#include "BT_LeafNodeCondition.h"
#include "BehaviourTree.h"

BT_LeafNodeCondition::BT_LeafNodeCondition(BehaviourTree* aTree)
	: BT_Node(aTree)
{
}

BT_NodeStatus BT_LeafNodeCondition::Execute()
{
	bool condition = myCondition();

	myTree->AddConditionNodeToCheck(this, condition);

	return condition ? BT_NodeStatus::Success : BT_NodeStatus::Failure;
}

void BT_LeafNodeCondition::SetCondition(const std::function<bool()>&& aCondition)
{
	myCondition = aCondition;
}

bool BT_LeafNodeCondition::GetCondition()
{
	return myCondition();
}
