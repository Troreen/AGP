#pragma once
#include "BT_Node.h"
#include <functional>

class BT_LeafNodeAction : public BT_Node
{
public:
	BT_LeafNodeAction() = delete;
	BT_LeafNodeAction(BehaviourTree* aTree);

	BT_NodeStatus Execute() override;
	void Update();

	void SetEnter(std::function<void()>&& aEnter);
	void SetCondition(std::function<BT_NodeStatus()>&& aCondition);
	void SetStart(std::function<void()>&& aEnter);
	void SetAction(std::function<void()>&& aAction);
	void SetEnd(std::function<void()>&& aEnter);

	void Exit();

private:
	std::function<void()> myEnter;
	std::function<BT_NodeStatus()> myCondition;
	std::function<void()> myStart;
	std::function<void()> myAction;
	std::function<void()> myEnd;
};
