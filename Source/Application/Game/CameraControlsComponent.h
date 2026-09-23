#pragma once
#include "GameFramework/World/Component.h"
#include "InputMapper.h"
#include "Vector2.hpp"

#include <vector>

// Provides free-fly movement and mouse-look controls for the owning Actor.
class CameraControlsComponent final : public Component
{
public:
	void BeginPlay() override;
	void EndPlay() noexcept override;
	void Update(float deltaTime) override;

private:
	void BindHeldInput(CommonUtilities::InputMapper& aInput, std::string_view aAction, bool& aState);

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
	std::vector<unsigned> myListenerIDs;
};
