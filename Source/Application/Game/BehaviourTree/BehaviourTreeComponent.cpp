#include "BehaviourTreeComponent.h"
#include <GameFramework/Components/AnimatorComponent.h>
#include <GameFramework/World/Actor.h>

BehaviourTreeComponent::BehaviourTreeComponent()
	: myFollow(&myBehaviourTree.GetBlackBoard())
	, myFind(&myBehaviourTree)
	, myIdle(&myBehaviourTree.GetBlackBoard())
{
	myLooking.AddChild(&myFind);
	myLooking.AddChild(&myFollow);

	myRoot.AddChild(&myLooking);
	myRoot.AddChild(&myIdle);

	myBehaviourTree.SetRoot(&myRoot);
	myActionNode = &myIdle;
}

void BehaviourTreeComponent::SetBehaviourTree(BehaviourTree* /*aBehaviourTree*/)
{
	//myBehaviourTree = aBehaviourTree;
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
