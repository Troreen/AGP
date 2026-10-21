#include "BehaviourTreeComponent.h"
#include <GameFramework/Components/AnimatorComponent.h>
#include <GameFramework/World/Actor.h>
#include <BehaviourTree/BehaviourTreeFactory.h>

BehaviourTreeComponent::BehaviourTreeComponent()
{
	myActionNode = nullptr;
}

void BehaviourTreeComponent::SetBehaviourTree(const char* aBehaviourTree)
{
	BehaviourTreeFactory factory;
	factory.Init();
	myActionNode = factory.GetBehaviourTree(aBehaviourTree, myBehaviourTree);
}

void BehaviourTreeComponent::SetActionNode(BT_LeafNodeAction* aAtionNode)
{
	if (myActionNode)
	{
		myActionNode->EarlyExit();
	}

	myActionNode = aAtionNode;
}

void BehaviourTreeComponent::InitializeCustomVariables(const CustomComponentVariables& someVariables)
{
	someVariables;
}

void BehaviourTreeComponent::BeginPlay()
{
	myBehaviourTree.GetBlackBoard().SetActor(BlackBoardVariable_Actors::OwnerActor, GetOwner());
	myBehaviourTree.GetBlackBoard().SetComponent(BlackBoardVariable_Components::OwnerAnimation, GetOwner()->GetComponent<AnimatorComponent>());
}

void BehaviourTreeComponent::Update(float aDeltaTime)
{
	PreUpdate();


	UpdateData();
}

void BehaviourTreeComponent::PreUpdate()
{
	myBehaviourTree.Run();

	UpdateTree();
}

void BehaviourTreeComponent::UpdateTree()
{
	if (myActionNode->Tick() != BT_NodeStatus::Running)
	{
		myBehaviourTree.Tick();
	}
}

void BehaviourTreeComponent::UpdateData()
{
	myActionNode->UpdateActorControllerData();
	myActionNode->TriggerAction(*this->GetOwner());
}

void BehaviourTreeComponent::UpdateMovement()
{

}
