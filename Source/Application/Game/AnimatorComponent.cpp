#include "AnimatorComponent.h"
#include <GameFramework/ServiceLocator.h>
#include "AnimationManager.h"

AnimatorComponent::AnimatorComponent()
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
