#pragma once
#include "ActionNode.h"

class FallbackNode : public ActionNode
{
public:
	FallbackNode() = default;
	~FallbackNode() = default;

	NodeStatus Tick() override;
};