#include "GameFramework/Components/DebugCameraController.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/CameraComponent.h"
#include "Maths.hpp"
#include <algorithm>
#include <cmath>

const DebugCameraPreset& GetDebugCameraPreset()
{
	static const DebugCameraPreset preset;
	return preset;
}

CameraComponent* DebugCameraService::Ensure(World& world, CommonUtilities::Vector2u clientSize)
{
	constexpr const char* actorName = "__DebugCamera";
	if (auto* existing = world.FindActor(actorName))
		if (auto* camera = existing->GetComponent<CameraComponent>()) 
			return camera;
	auto* actor = world.SpawnActor(actorName); actor->GetTransform().SetData(GetDebugCameraPreset().Transform);
	auto* camera = actor->AddComponent<CameraComponent>("Camera", GetDebugCameraPreset().FieldOfView, GetDebugCameraPreset().NearPlane,
	                                                  GetDebugCameraPreset().FarPlane, clientSize);
	actor->AddComponent<DebugCameraController>("Controls");
	return camera;
}

void DebugCameraService::Toggle(World& world, CommonUtilities::Vector2u clientSize)
{
	auto* active = world.GetActiveCamera();
	if (active && active->GetOwner()->GetName() == "__DebugCamera")
	{
		auto* actor = world.FindActor(myPreviousActor);
		auto* camera = actor ? dynamic_cast<CameraComponent*>(actor->FindComponent(myPreviousComponent)) : nullptr;
		if (camera) 
			world.SetActiveCamera(camera);
		return;
	}
	if (active) 
	{ 
		myPreviousActor = active->GetOwner()->GetName(); myPreviousComponent = active->GetName(); 
	}
	world.SetActiveCamera(Ensure(world, clientSize));
}

void DebugCameraService::Reset() 
{ 
	myPreviousActor.clear(); 
	myPreviousComponent.clear(); 
}

void DebugCameraController::BeginPlay()
{
	const auto forward = CU::NormalizeSafe(GetOwner()->GetTransform().GetLocalForward(), CU::Vector3f::UnitZ);
	myYaw = std::atan2(forward.x, forward.z);
	myPitch = -std::asin(CU::Clamp(forward.y, -1.f, 1.f));
	auto& input = GetInputSystem();
	auto held = [this, &input](const InputActionId& action, bool& target)
	{
		mySubscriptions.push_back(input.Subscribe(action, [&target](const InputActionEvent& event) { target = event.Phase != InputActionPhase::Ended; }));
	};
	held(InputActions::CameraLookEnable, myLook); held(InputActions::CameraForward, myForward); held(InputActions::CameraBack, myBack);
	held(InputActions::CameraLeft, myLeft); held(InputActions::CameraRight, myRight); held(InputActions::CameraUp, myUp); held(InputActions::CameraDown, myDown);
	mySubscriptions.push_back(input.Subscribe(InputActions::CameraLookDelta, [this](const InputActionEvent& event)
	{
		if (event.Phase != InputActionPhase::Ended) 
			myLookDelta += std::get<CommonUtilities::Vector2f>(event.Value);
	}));
}

void DebugCameraController::Update(float deltaTime)
{
	auto& transform = GetOwner()->GetTransform();
	const auto& preset = GetDebugCameraPreset();
	if (myLook)
	{
		myYaw += myLookDelta.x * preset.LookSensitivity;
		myPitch = CU::Clamp(myPitch + myLookDelta.y * preset.LookSensitivity, CU::DegreesToRadians(-89.0f), CU::DegreesToRadians(89.0f));
		transform.SetLocalRotationDegrees(CU::RadiansToDegrees(myYaw), CU::RadiansToDegrees(myPitch), 0);
	}
	myLookDelta = {};
	CommonUtilities::Vector3f motion{};
	if (myForward)
		motion += CU::NormalizeSafe(transform.GetLocalForward()); if (myBack) motion -= CU::NormalizeSafe(transform.GetLocalForward());
	if (myRight)
		motion += CU::NormalizeSafe(transform.GetLocalRight()); if (myLeft) motion -= CU::NormalizeSafe(transform.GetLocalRight());
	if (myUp) 
		motion += CommonUtilities::Vector3f::UnitY; if (myDown) motion -= CommonUtilities::Vector3f::UnitY;
	if (motion.LengthSqr() > 0) 
		transform.SetLocalPosition(transform.GetLocalPosition() + CU::NormalizeSafe(motion) * preset.MoveSpeed * deltaTime);
}
