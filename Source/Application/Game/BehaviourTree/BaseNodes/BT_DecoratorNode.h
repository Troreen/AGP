#pragma once
#include "BT_Node.h"
#include <memory>

class BT_DecoratorNode : public BT_Node
{
public:
	BT_DecoratorNode() = default;

	void SetChild(std::shared_ptr<BT_Node> aChild) { myChild = aChild; }

protected:
	std::shared_ptr<BT_Node> myChild;
};
