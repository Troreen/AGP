#include "BT_CompositeNode.h"

BT_CompositeNode::BT_CompositeNode(BehaviourTree* aTree)
	: BT_Node(aTree)
{
}

void BT_CompositeNode::AddChild(std::shared_ptr<BT_Node> aChild)
{
	myChildren.push_back(aChild);
}
