#pragma once
#include "GameFramework/Runtime/InputSystem.h"
#include "GameFramework/World/Component.h"
#include "GameFramework/World/Transform.h"
#include <vector>

class CameraComponent;
class World;

// TODO!: Debug camera controller is controlling all debug cameras in the scene which is okay since we should not have 2 debug cameras active at the same time but still needs fixing of somesort 

struct DebugCameraPreset
{
	TransformData Transform{{0, 260, -950}, {0, 0, 0}, {1, 1, 1}};
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
	void Update(float deltaTime) override;
private:
	float myYaw = 0, myPitch = 0;
	CommonUtilities::Vector2f myLookDelta{};

	bool myLook 	= false; 
	bool myForward 	= false;
	bool myBack 	= false; 
	bool myLeft 	= false; 
	bool myRight 	= false; 
	bool myUp 		= false; 
	bool myDown 	= false;

	std::vector<InputSubscription> mySubscriptions;
};
