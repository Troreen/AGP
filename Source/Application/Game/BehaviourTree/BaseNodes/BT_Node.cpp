#include "BT_Node.h"

BT_Node::BT_Node()
{
	myStatus = BT_NodeStatus::Invalid;
}

BT_NodeStatus BT_Node::Tick()
{
	if (myStatus != BT_NodeStatus::Running)
	{
		Init();
	}

	myStatus = Update();

	return myStatus;
}
