#pragma once
#include "BT_CompositeNode.h"

class BT_SequenceNode : public BT_CompositeNode
{
public:
	BT_NodeStatus Update() override;
};
