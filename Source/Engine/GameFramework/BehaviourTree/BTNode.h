#pragma once

enum class NodeStatus
{
	Success,
	Running,
	Failure
};

class BTNode
{
public:

	BTNode() = default;
	~BTNode() = default;

	virtual NodeStatus Tick() = 0;
};