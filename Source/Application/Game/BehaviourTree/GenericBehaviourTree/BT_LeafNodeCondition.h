#pragma once
#include "BT_Node.h"
#include <memory>
#include <functional>

class BT_LeafNodeCondition : public BT_Node
{
public:
	BT_LeafNodeCondition() = delete;
	BT_LeafNodeCondition(BehaviourTree* aTree);

	BT_NodeStatus Execute() override;

	void SetCondition(const std::function<bool()>&& aCondition);
	bool GetCondition();

private:
	std::function<bool()> myCondition;
};
