#pragma once

#include <GameFramework/Components/CustomComponent.h>
#include "GenericBehaviourTree/BehaviourTree.h"

class BT_LeafNodeAction;

class BehaviourTreeComponent : public CustomComponent
{
public:

	void InitializeCustomVariables(const CustomComponentVariables& someVariables) override;

	void BeginPlay() override;

	void Update(float aDeltaTime) override;
private:

	template <typename T>
	void BlackBoardInsertVariables(BlackBoard& aBB, const std::unordered_map<std::string, T>& aMap, const std::string& aSkipInsert = "");

	BehaviourTree myTree;
	BT_LeafNodeAction* myActionNode;
	std::string myTreeName;
};
