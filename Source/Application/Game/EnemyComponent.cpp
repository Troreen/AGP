#include "EnemyComponent.h"

EnemyComponent::EnemyComponent()
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

void EnemyComponent::SetBehaviourTree(BehaviourTree* /*aBehaviourTree*/)
{
	//myBehaviourTree = aBehaviourTree;
}

void EnemyComponent::SetActionNode(BT_LeafNodeAction* aAtionNode)
{
	myActionNode = aAtionNode;
}

void EnemyComponent::BeginPlay()
{
	myBehaviourTree.GetBlackBoard().SetActor(BlackBoardVariable_Actors::OwnerActor, GetOwner());
}

void EnemyComponent::Update(float aDeltaTime)
{
	PreUpdate();

	UpdateComponent();
}

void EnemyComponent::PreUpdate()
{
	myBehaviourTree.Run();
}

void EnemyComponent::UpdateComponent()
{
	myActionNode->Update();
}
