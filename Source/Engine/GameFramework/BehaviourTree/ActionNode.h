#pragma once
#include "BTNode.h"
#include <vector>

class ActionNode : public BTNode
{
public:
	ActionNode() = default;
	~ActionNode() = default;

	virtual NodeStatus Tick() override = 0;
protected:
	std::vector<BTNode> myNodes;
};