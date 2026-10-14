#pragma once
#include "BT_Node.h"
#include <vector>

class BehaviourTree;

class BT_CompositeNode : public BT_Node
{
public:
	void AddChild(BT_Node* aChild) { myChildren.push_back(aChild); }
protected:
	std::vector<BT_Node*> myChildren;
};
