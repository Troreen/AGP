#pragma once
#include "BT_Node.h"

class BT_DecoratorNode : public BT_Node
{
public:
	BT_DecoratorNode() = default;

	void SetChild(BT_Node* aChild) { myChild = aChild; }

protected:
	BT_Node* myChild;
};