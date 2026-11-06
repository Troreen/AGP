#pragma once
#include <string>
#include <unordered_map>
#include <functional>


class BehaviourTree;

class BehaviourTreeFactory
{
public:

	BehaviourTreeFactory();

	static BehaviourTreeFactory& Instance()
	{
		static BehaviourTreeFactory instance;
		return instance;
	}

	void SetBehaviourTree(const std::string& aTree, BehaviourTree& outTree);

private:

	std::unordered_map<std::string, std::function<void(BehaviourTree&)>> myBehaviourTrees;
};
