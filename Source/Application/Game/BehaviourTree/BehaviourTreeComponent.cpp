#include "BehaviourTreeComponent.h"
#include "GenericBehaviourTree/BT_LeafNodeAction.h"
#include "BehaviourTreeFactory.h"
#include <GameFramework/Scenes/CustomComponentVariables.h>
#include <iostream>
#include <GameFramework/World/Actor.h>


template <typename T>
void BehaviourTreeComponent::BlackBoardInsertVariables(BlackBoard& aBB, const std::unordered_map<std::string, T>& aMap, const std::string& aSkipInsert)
{
	for (auto& element : aMap)
	{
		if (element.first == aSkipInsert)
		{
			continue;
		}

		aBB.Set<const T>(element.first, std::move(element.second));
	}
}


void BehaviourTreeComponent::InitializeCustomVariables(const CustomComponentVariables& someVariables)
{
	myTreeName = someVariables.GetOrDefault<std::string>("BehaviourTree", "");

	BlackBoard& bb = myTree.GetBlackBoard();

	BlackBoardInsertVariables<int64_t>(bb, someVariables.IntVariables);
	BlackBoardInsertVariables<float>(bb, someVariables.FloatVariables);
	BlackBoardInsertVariables<bool>(bb, someVariables.BoolVariables);
	BlackBoardInsertVariables<std::string>(bb, someVariables.StringVariables, "BehaviourTree");
	BlackBoardInsertVariables<CommonUtilities::Vector3f>(bb, someVariables.VectorVariables);
	BlackBoardInsertVariables<CustomTransformData>(bb, someVariables.TransformVariables);

	if (myTreeName.empty())
	{
		std::cout << "No tree defined on " << GetOwner()->GetName() << " actor";
	}
}

void BehaviourTreeComponent::BeginPlay()
{
	myTree.GetBlackBoard().Set<Actor*>("Owner", GetOwner());
	BehaviourTreeFactory().SetBehaviourTree(myTreeName, myTree);
	myActionNode = myTree.GetBlackBoard().Get<BT_LeafNodeAction*>("Running");
}

void BehaviourTreeComponent::Update(float aDeltaTime)
{
	//myTree.Run();

	myActionNode = myTree.GetBlackBoard().Get<BT_LeafNodeAction*>("Running");

	if (myActionNode)
	{
		myActionNode->Update();
	}
}
