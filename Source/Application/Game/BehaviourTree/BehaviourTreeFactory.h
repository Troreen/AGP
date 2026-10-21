#pragma once
#include "BehaviourTree.h"
#include <unordered_map>
#include <memory>
#include <string>
#include "BaseNodes/BT_Node.h"
#include "BaseNodes/BT_LeafNodeAction.h"
#include <functional>

class BehaviourTreeFactory
{
public:
	void Init();

	BT_LeafNodeAction* GetBehaviourTree(std::string aBehaviourTreeType, BehaviourTree & inoutBehaviourTree);

private:

	using CreateBehaviourTree = std::function<BT_LeafNodeAction*(BehaviourTree&)>;

	std::unordered_map<std::string, CreateBehaviourTree> myBehaviourTrees;
};
