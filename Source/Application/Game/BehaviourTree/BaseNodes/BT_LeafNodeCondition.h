#pragma once
#include "BT_Node.h"
#include <memory>

class BlackBoard;
class BehaviourTree;

class BT_LeafNodeCondition : public BT_Node
{
public:
	BT_LeafNodeCondition() = delete;
	BT_LeafNodeCondition(BehaviourTree* aBehaviourTree);

	BT_NodeStatus Update() override;

	virtual bool CheckCondition() const = 0;

protected:
	BlackBoard* myBlackBoard;

private:
	BehaviourTree* myBehaviourTree;
};
