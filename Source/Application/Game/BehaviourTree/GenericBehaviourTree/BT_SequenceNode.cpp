#include "BT_SequenceNode.h"
#include "BehaviourTree.h"

BT_NodeStatus BT_SequenceNode::Execute()
{
	for (size_t childIndex = 0; childIndex < myChildren.size(); childIndex++)
	{
		switch (myChildren[childIndex]->Execute())
		{
		case BT_NodeStatus::Running:
			return BT_NodeStatus::Running;

		case BT_NodeStatus::Failure:
			return BT_NodeStatus::Failure;
		}
	}

    return BT_NodeStatus::Success;
}
