#pragma once
#include "BT_Node.h"
#include <vector>
#include <memory>

class BehaviourTree;

class BT_CompositeNode : public BT_Node
{
public:
	BT_CompositeNode() = delete;
	BT_CompositeNode(BehaviourTree* aTree);

	void AddChild(std::shared_ptr<BT_Node> aChild);
protected:
	std::vector<std::shared_ptr<BT_Node>> myChildren;
};
