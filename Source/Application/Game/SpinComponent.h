#pragma once
#include "GameFramework/World/Component.h"
#include "GameFramework/Runtime/InputSystem.h"

#include <string>

class Transform;

// Continuously rotates the owning Actor or a named SceneComponent.
class SpinComponent final : public Component
{
public:
	void SetTargetComponentName(const char* aComponentName);
	void BeginPlay() override;
	void Update(float deltaTime) override;

private:
	Transform* FindTargetTransform() const;
	std::string myTargetComponentName;
	float myYaw = 0;
	bool mySpinning = true;
	InputSubscription myToggleSubscription;
};
