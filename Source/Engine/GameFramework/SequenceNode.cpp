#include "SequenceNode.h"

NodeStatus SequenceNode::Tick()
{
	for (auto& node : myNodes)
	{
		NodeStatus status = node.Tick();

		if (status == NodeStatus::Failure)
		{
			return status;
		}
	}

	return NodeStatus::Success;
}
