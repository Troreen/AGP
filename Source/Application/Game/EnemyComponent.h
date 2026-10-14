#pragma once
#include <GameFramework/World/Component.h>

#include <BehaviourTree/BehaviourTree.h>
#include <BehaviourTree/BaseNodes/BT_LeafNodeAction.h>

#include <BehaviourTree/CustomNodes/ActionNodes/BT_Action_FollowPlayer.h>
#include <BehaviourTree/CustomNodes/ActionNodes/BT_Action_Idle.h>
#include <BehaviourTree/CustomNodes/ConditionNodes/BT_Condition_IsPlayerInLOS.h>
#include <BehaviourTree/BaseNodes/BT_SelectorNode.h>
#include <BehaviourTree/BaseNodes/BT_SequenceNode.h>

class EnemyComponent : public Component
{
public:

	EnemyComponent();

	void SetBehaviourTree(BehaviourTree* aBehaviourTree);
	void SetActionNode(BT_LeafNodeAction* aActionNode);

	void BeginPlay() override;
	void Update(float aDeltaTime) override;

private:

	void PreUpdate();
	void UpdateComponent();
	//void AddSteeringForce();
	//void TriggerAction();

	BT_Action_FollowPlayer myFollow;
	BT_Action_Idle myIdle;

	BT_Condition_IsPlayerInLOS myFind;

	BT_SequenceNode myLooking;
	BT_SelectorNode myRoot;

	BehaviourTree myBehaviourTree;
	BT_LeafNodeAction* myActionNode;
};
