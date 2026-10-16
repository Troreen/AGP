#include "BT_Node.h"

BT_Node::BT_Node()
{
	myStatus = BT_NodeStatus::Invalid;
}

BT_NodeStatus BT_Node::Tick()
{
	if (myStatus != BT_NodeStatus::Running)
	{
		OnEnter();
	}

	myStatus = Update();

	if (myStatus != BT_NodeStatus::Running)
	{
		OnExit();
	}

	return myStatus;
}

void BT_Node::EarlyExit()
{
	myStatus = BT_NodeStatus::EarlyExit;
	OnExit();
}
