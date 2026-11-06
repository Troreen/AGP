#pragma once
#include "BT_CompositeNode.h"

class BT_SequenceNode : public BT_CompositeNode
{
public:
	BT_SequenceNode() = delete;
	BT_SequenceNode(BehaviourTree* aTree) : BT_CompositeNode(aTree) {}

	BT_NodeStatus Execute() override;
};
