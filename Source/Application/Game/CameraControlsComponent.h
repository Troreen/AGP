#pragma once
#include "GameFramework/World/Component.h"
#include "GameFramework/Runtime/InputSystem.h"

#include <vector>

// Provides free-fly movement and mouse-look controls for the owning Actor.
class CameraControlsComponent final : public Component
{
public:
	void BeginPlay() override;
	void Update(float deltaTime) override;

private:
	void BindHeldInput(InputSystem& aInput, const InputActionId& aAction, bool& aState);

	float myYaw = 0;
	float myPitch = 0;
	CommonUtilities::Vector2f myLookDelta{};
	bool myLookActive = false;
	bool myMoveForward = false;
	bool myMoveBack = false;
	bool myMoveLeft = false;
	bool myMoveRight = false;
	bool myMoveUp = false;
	bool myMoveDown = false;
	std::vector<InputSubscription> mySubscriptions;
};
