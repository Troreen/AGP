#pragma once
#include "GameFramework/World/Component.h"
#include "GameFramework/Runtime/InputSystem.h"

#include <string>
#include <vector>

// Handles input for inspecting and manipulating scene lights.
// Actors are stored by name so they can be safely resolved after scene changes.
class LightControlsComponent final : public Component
{
public:
	void BeginPlay() override;
	void Update(float deltaTime) override;

	std::string CameraName, DirectionalName, PointName, SpotName;

private:
	enum Request : unsigned
	{
		Print = 1,
		ToggleDir = 2,
		TogglePoints = 4,
		ToggleSpot = 8,
		AimDir = 16,
		PlacePoints = 32,
		PlaceSpot = 64
	};

	unsigned myRequests = 0;
	std::vector<InputSubscription> mySubscriptions;
};
