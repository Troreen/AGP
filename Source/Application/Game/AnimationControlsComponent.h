#pragma once
#include "GameFramework/World/Component.h"
#include "GameFramework/Runtime/InputSystem.h"

#include <vector>

// Binds input actions to animation playback on the owner's SkeletalMeshComponent.
class AnimationControlsComponent final : public Component
{
public:
	void BeginPlay() override;

private:
	void BindAnimationInput(const InputActionId& aAction, const char* aAnimationName, bool aPlayPartial);
	std::vector<InputSubscription> mySubscriptions;
};
