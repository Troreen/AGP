#pragma once
#include "BT_Node.h"
#include <vector>
#include <memory>

class BehaviourTree;

class BT_CompositeNode : public BT_Node
{
public:
	void AddChild(std::shared_ptr<BT_Node> aChild) { myChildren.push_back(aChild); }
protected:
	std::vector<std::shared_ptr<BT_Node>> myChildren;
};
