#include "AnimationControlsComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/World/Actor.h"

#include <utility>

void AnimationControlsComponent::BeginPlay()
{
    BindAnimationInput(InputActions::PlayBreathing, "Breathing", false);
    BindAnimationInput(InputActions::PlayWalk, "Walk", false);
    BindAnimationInput(InputActions::PlayRun, "Run", false);
    BindAnimationInput(InputActions::PlayWave, "Wave", true);
}

void AnimationControlsComponent::BindAnimationInput(
	const InputActionId& aAction, const char* aAnimationName, bool aPlayPartial)
{
	InputSubscription inputSubscription = GetInputSystem().Subscribe(
		aAction, 
		[this, aAnimationName, aPlayPartial](const InputActionEvent& event)
		{
			if (event.Phase != InputActionPhase::Started) 
			{
				return;
			}

			SkeletalMeshComponent* mesh = GetOwner()->GetComponent<SkeletalMeshComponent>();
			if (mesh == nullptr)
			{
				return;
			}

			if (aPlayPartial)
			{
				const bool partialPlayed = mesh->PlayPartialAnimation(aAnimationName, false);
				if (partialPlayed)
				{
					return;
				}
			}

			mesh->PlayAnimation(aAnimationName, !aPlayPartial);
		});
	mySubscriptions.push_back(std::move(inputSubscription));
}
