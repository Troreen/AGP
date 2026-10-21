#pragma once
#include <GameFramework/Components/CustomComponent.h>

#include <BehaviourTree/BehaviourTree.h>
#include <BehaviourTree/BaseNodes/BT_LeafNodeAction.h>

#include <BehaviourTree/CustomNodes/ActionNodes/BT_Action_FollowPlayer.h>
#include <BehaviourTree/CustomNodes/ActionNodes/BT_Action_Idle.h>
#include <BehaviourTree/CustomNodes/ConditionNodes/BT_Condition_IsPlayerInLOS.h>
#include <BehaviourTree/BaseNodes/BT_SelectorNode.h>
#include <BehaviourTree/BaseNodes/BT_SequenceNode.h>

class BehaviourTreeComponent : public CustomComponent
{
public:

	BehaviourTreeComponent();

	void SetBehaviourTree(const char* aBehaviourTree);
	void SetActionNode(BT_LeafNodeAction* aActionNode);

	void InitializeCustomVariables(const CustomComponentVariables& someVariables) override;

	void BeginPlay() override;
	void Update(float aDeltaTime) override;

private:

	void PreUpdate();
	void UpdateTree();
	void UpdateData();
	void UpdateMovement();
	//void AddSteeringForce();
	//void TriggerAction();

	BehaviourTree myBehaviourTree;
	BT_LeafNodeAction* myActionNode;
};
