#pragma once
#include "BT_CompositeNode.h"

class BT_SelectorNode : public BT_CompositeNode
{
public:
	BT_SelectorNode() = delete;
	BT_SelectorNode(BehaviourTree* aTree) : BT_CompositeNode(aTree) {}

	BT_NodeStatus Execute() override;
};
