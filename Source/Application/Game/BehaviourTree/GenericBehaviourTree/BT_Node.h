#pragma once
#include <type_traits>

class BehaviourTree;
class BlackBoard;

enum class BT_NodeStatus
{
	Running,
	Success,
	Failure
};

class BT_Node
{
public:
	BT_Node() = delete;
	BT_Node(BehaviourTree* aTree);

	virtual BT_NodeStatus Execute();

	BlackBoard* GetBlackBoard();

protected:
	BehaviourTree* myTree;
};
