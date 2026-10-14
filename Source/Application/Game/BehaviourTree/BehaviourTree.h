#pragma once
#include "BaseNodes/BT_Node.h"
#include "BaseNodes/BT_LeafNodeCondition.h"
#include "BaseNodes/BT_CompositeNode.h"

#include "BlackBoard.h"
#include <vector>

class BehaviourTree
{
public:
	BehaviourTree();

	void SetRoot(BT_Node* aRoot) { myRoot = aRoot; }
	void Run();

	BlackBoard& GetBlackBoard() { return myBlackBoard; }

	void AddLeafNodeCondition(const BT_LeafNodeCondition* aLeafNodeCondition, bool aResultAtLastTick) { myCurrentLeafNodeConditionAndResultsAtLastTick.push_back({ aLeafNodeCondition, aResultAtLastTick }); }
	void Tick();
private:
	void CheckCurrentLeafNodeConditions();

	BT_Node* myRoot;
	BlackBoard myBlackBoard;
	bool myShouldTick;

	struct LeafNodeConditionAndResultAtLastTick
	{
		const BT_LeafNodeCondition* LeafNodeCondition = nullptr;
		bool ResultAtLastTick = false;
	};

	std::vector<LeafNodeConditionAndResultAtLastTick> myCurrentLeafNodeConditionAndResultsAtLastTick;
};
