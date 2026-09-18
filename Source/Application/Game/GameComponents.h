#pragma once
#include "GameFramework/World/Component.h"
#include "GameFramework/Runtime/InputSystem.h"

#include <vector>

class GameContext;
class DirectionalLightComponent;
class PointLightComponent;
class SpotLightComponent;
class SkeletalMeshComponent;
class Transform;

// Game behaviors run on the gameplay owner. Service getters are valid after attachment.
// The upright camera stores the Actor's orientation: yaw follows world up,
// and pitch follows the camera's yawed right axis without introducing roll.
class CameraControlsComponent final : public Component
{
public:
	void BeginPlay() override;
	void Update(float deltaTime) override;

private:
	float myYaw = 0;
	float myPitch = 0;
	CommonUtilities::Vector2f myLookDelta{};
	bool myLookActive = false;
	bool myForward = false, myBack = false, myLeft = false, myRight = false, myUp = false, myDown = false;
	std::vector<InputSubscription> mySubscriptions;
};

// Update example: issue playback requests to a sibling SkeletalMeshComponent.
// Attach before the mesh component so requests affect its Update in the same frame.
class AnimationControlsComponent final : public Component
{
public:
	void BeginPlay() override;
private:
	std::vector<InputSubscription> mySubscriptions;
};

// Update example: rotate using elapsed frame time.
// This demo assumes an initially unrotated chest and owns all of its rotation.
// R toggles spinning once per input press.
class SpinComponent final : public Component
{
public:
	void SetTargetComponentName(std::string componentName) { myTargetComponentName = std::move(componentName); }
	void BeginPlay() override;
	void Update(float deltaTime) override;

private:
	Transform* FindTargetTransform() const;
	std::string myTargetComponentName;
	float myYaw = 0;
	bool mySpinning = true;
	InputSubscription myToggleSubscription;
};

// Attached to a scene-controls actor after the camera so Update uses its final pose.
// Cross-actor behavior example: references are wired during scene creation.
// Store names and look up each frame; no pointers survive destruction or scene replacement.
class LightControlsComponent final : public Component
{
public:
	void BeginPlay() override;
	void Update(float deltaTime) override;
	std::string CameraName, DirectionalName, PointName, SpotName;
private:
	enum Request : unsigned { Print = 1, ToggleDir = 2, TogglePoints = 4, ToggleSpot = 8, AimDir = 16, PlacePoints = 32, PlaceSpot = 64 };
	unsigned myRequests = 0;
	std::vector<InputSubscription> mySubscriptions;
};
