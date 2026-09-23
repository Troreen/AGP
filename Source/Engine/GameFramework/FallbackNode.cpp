#include "FallbackNode.h"

NodeStatus FallbackNode::Tick()
{
	for (auto& node : myNodes)
	{
		NodeStatus status = node.Tick();

		if (status == NodeStatus::Failure)
		{
			continue;
		}

		return status;
	}
}
