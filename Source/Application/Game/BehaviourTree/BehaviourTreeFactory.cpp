#include "BehaviourTreeFactory.h"

#include "CustomNodes/ActionNodes/BT_Action_FollowPlayer.h"
#include "CustomNodes/ActionNodes/BT_Action_Idle.h"
#include "CustomNodes/ConditionNodes/BT_Condition_IsPlayerInLOS.h"

#include "BaseNodes/BT_SelectorNode.h"
#include "BaseNodes/BT_SequenceNode.h"


void BehaviourTreeFactory::Init()
{
	myBehaviourTrees["Test"] = [](BehaviourTree& inoutBehaviourTree)
	{
		std::shared_ptr<BT_Action_FollowPlayer> followPlayer = std::make_shared<BT_Action_FollowPlayer>(&inoutBehaviourTree.GetBlackBoard());
		std::shared_ptr<BT_Action_Idle> idle = std::make_shared<BT_Action_Idle>(&inoutBehaviourTree.GetBlackBoard());
		std::shared_ptr<BT_Condition_IsPlayerInLOS> playerInLOS = std::make_shared<BT_Condition_IsPlayerInLOS>(&inoutBehaviourTree);

		std::shared_ptr<BT_SelectorNode> root = std::make_shared<BT_SelectorNode>();
		std::shared_ptr<BT_SequenceNode> find = std::make_shared<BT_SequenceNode>();

		root->AddChild(find);
		root->AddChild(idle);

		find->AddChild(playerInLOS);
		find->AddChild(followPlayer);

		inoutBehaviourTree.SetRoot(root);
		return reinterpret_cast<BT_LeafNodeAction*>(idle.get());
	};
}

BT_LeafNodeAction* BehaviourTreeFactory::GetBehaviourTree(std::string aBehaviourTreeType, BehaviourTree& inoutBehaviourTree)
{
	if (myBehaviourTrees.find(aBehaviourTreeType) == myBehaviourTrees.end())
	{
		return nullptr;
	}

	return myBehaviourTrees[aBehaviourTreeType](inoutBehaviourTree);
}
