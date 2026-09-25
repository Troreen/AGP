#include "AnimatorComponent.h"

#include "../Animation/AnimationManager.h"

#include <GameFramework/ServiceLocator.h>

AnimatorComponent::AnimatorComponent() : myMeshComponent(nullptr)
{
	myAnimationTree = new AnimationTree(ServiceLocator::GetInstance().GetAnimationManager().GetAnimationTree(""));
}

void AnimatorComponent::Update(float aDeltaTime)
{
	if (myAnimationTree)
	{
		myAnimationTree->Update(aDeltaTime);
	}
}
