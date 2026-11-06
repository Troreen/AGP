#include "BT_Node.h"
#include "BehaviourTree.h"
#include "BlackBoard.h"

BT_Node::BT_Node(BehaviourTree* aTree)
	: myTree(aTree)
{
}

BT_NodeStatus BT_Node::Execute()
{
	return BT_NodeStatus::Success;
}

BlackBoard* BT_Node::GetBlackBoard()
{
	return myTree ? &myTree->GetBlackBoard() : nullptr;
}
