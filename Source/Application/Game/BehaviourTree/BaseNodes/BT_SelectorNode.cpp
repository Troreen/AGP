#include "BT_SelectorNode.h"
#include <BehaviourTree/BehaviourTree.h>

BT_NodeStatus BT_SelectorNode::Update()
{
	for (size_t childIndex = 0; childIndex < myChildren.size(); childIndex++)
	{
		switch (myChildren[childIndex]->Tick())
		{
		case BT_NodeStatus::Invalid:
			return BT_NodeStatus::Invalid;

		case BT_NodeStatus::Running:
			return BT_NodeStatus::Running;

		case BT_NodeStatus::Success:
			return BT_NodeStatus::Success;
		}
	}

	return BT_NodeStatus::Failure;
}
