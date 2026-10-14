#pragma once
#include <BehaviourTree/BaseNodes/BT_LeafNodeCondition.h>
#include <BehaviourTree/BehaviourTree.h>

class BT_Condition_IsPlayerInLOS : public BT_LeafNodeCondition
{
public:
	BT_Condition_IsPlayerInLOS() = delete;
	BT_Condition_IsPlayerInLOS(BehaviourTree* aBehaviourTree);

	bool CheckCondition() const override;

private:
	float myLOSRadius;
};
