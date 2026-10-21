#pragma once
//#include "BaseNodes/BT_Node.h"
//#include "BaseNodes/BT_LeafNodeCondition.h"

#include "BlackBoard.h"
#include <vector>
#include <memory>

class BT_Node;
class BT_LeafNodeCondition;

class BehaviourTree
{
public:
	BehaviourTree();

	void SetRoot(std::shared_ptr<BT_Node> aRoot) { myRoot = aRoot; }
	void Run();

	BlackBoard& GetBlackBoard() { return myBlackBoard; }

	void AddLeafNodeCondition(const BT_LeafNodeCondition* aLeafNodeCondition, bool aResultAtLastTick) { myCurrentLeafNodeConditionAndResultsAtLastTick.push_back({ aLeafNodeCondition, aResultAtLastTick }); }
	void Tick();
private:
	void CheckCurrentLeafNodeConditions();

	std::shared_ptr<BT_Node> myRoot;
	BlackBoard myBlackBoard;
	bool myShouldTick;

	struct LeafNodeConditionAndResultAtLastTick
	{
		const BT_LeafNodeCondition* LeafNodeCondition = nullptr;
		bool ResultAtLastTick = false;
	};

	std::vector<LeafNodeConditionAndResultAtLastTick> myCurrentLeafNodeConditionAndResultsAtLastTick;
};
