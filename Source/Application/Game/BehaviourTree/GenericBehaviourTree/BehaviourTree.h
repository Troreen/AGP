#pragma once
#include "BlackBoard.h"
#include <vector>
#include <memory>

class BT_Node;
class BT_LeafNodeCondition;

class BehaviourTree
{
public:
	BehaviourTree();
	~BehaviourTree() = default;
	BehaviourTree(const BehaviourTree&) = delete;
	BehaviourTree& operator=(const BehaviourTree&) = delete;
	BehaviourTree(BehaviourTree&&) = delete;
	BehaviourTree& operator=(const BehaviourTree&&) = delete;

	void SetRoot(std::shared_ptr<BT_Node> aRoot) { myRoot = aRoot; }
	void Run();

	BlackBoard& GetBlackBoard();

	void AddConditionNodeToCheck(BT_LeafNodeCondition* aConditionNode, bool aResult);

	void Tick();

private:
	void CheckForChangeInCondition();

	struct ConditionNodeToCheckForChange
	{
		BT_LeafNodeCondition* ConditionNodeToCheck = nullptr;
		bool Condition = false;
	};

	BlackBoard myBlackBoard;
	std::shared_ptr<BT_Node> myRoot;

	std::vector<ConditionNodeToCheckForChange> myConditionNodesToCheckForChange;
	bool myTick;
};
