#pragma once
#include "InputMapper.h"
#include "Vector2.hpp"
#include "GameFramework/World/Component.h"
#include "GameFramework/World/Transform.h"
#include <vector>

class CameraComponent;
class World;

// The service assumes one debug camera per World; supporting multiple cameras would
// require tracking the previous active camera separately for each debug camera.

struct DebugCameraPreset
{
	CommonUtilities::Vector3f Position = { 0, 260, -950 };
	CommonUtilities::Quaternion<float> Rotation = CommonUtilities::Quaternion<float>::CreateIdentity();
	CommonUtilities::Vector3f Scale = { 1, 1, 1 };
	float FieldOfView = 90;
	float NearPlane = 1;
	float FarPlane = 50000;
	float MoveSpeed = 500;
	float LookSensitivity = .0025f;
};

const DebugCameraPreset& GetDebugCameraPreset();

class DebugCameraService
{
public:
	CameraComponent* Ensure(World& world, CommonUtilities::Vector2u clientSize);
	void Toggle(World& world, CommonUtilities::Vector2u clientSize);
	void Reset();

private:
	std::string myPreviousActor;
	std::string myPreviousComponent;
};

class DebugCameraController final : public Component
{
public:
	void BeginPlay() override;
	void EndPlay() noexcept override;
	void Update(float deltaTime) override;

private:
	std::vector<unsigned> myListenerIDs;

	CommonUtilities::Vector3f myInput{};
	CommonUtilities::Vector2f myLookDelta{};

	float myYaw = 0;
	float myPitch = 0;

	bool myLook = false;
};
