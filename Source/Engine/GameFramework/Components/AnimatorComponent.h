#pragma once
#include "../Animation/AnimationTree.h"

#include <GameFramework/World/Component.h>
#include <GameFramework/Components/SkeletalMeshComponent.h>

class AnimatorComponent : public Component
{
public:
	AnimatorComponent();

	void Update(float aDeltaTime) override;

	void SetMeshComponent(SkeletalMeshComponent* skel) { myMeshComponent = skel; }

private:
	SkeletalMeshComponent* myMeshComponent;
	AnimationTree* myAnimationTree;
};
