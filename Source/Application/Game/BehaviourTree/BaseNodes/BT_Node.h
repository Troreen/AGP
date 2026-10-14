#pragma once

enum class BT_NodeStatus
{
	Invalid,
	Running,
	Success,
	Failure
};

class BT_Node
{
public:
	BT_Node();

	virtual void Init() {};

	BT_NodeStatus Tick();

protected:
	virtual BT_NodeStatus Update() = 0;

private:
	BT_NodeStatus myStatus;
};
