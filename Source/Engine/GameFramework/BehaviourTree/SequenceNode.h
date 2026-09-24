#pragma once
#include "ActionNode.h"

class SequenceNode : public ActionNode
{
public:
	SequenceNode() = default;
	~SequenceNode() = default;

	NodeStatus Tick() override;
};
