#pragma once

enum class BT_NodeStatus
{
	Invalid,
	EarlyExit,
	Running,
	Success,
	Failure
};

class BT_Node
{
public:
	BT_Node();

	virtual void OnEnter() {};
	virtual void OnExit() {};

	BT_NodeStatus Tick();
	void EarlyExit();

protected:
	virtual BT_NodeStatus Update() = 0;

private:
	BT_NodeStatus myStatus;
};
