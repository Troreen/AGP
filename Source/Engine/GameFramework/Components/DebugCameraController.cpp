#include "GameFramework.pch.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/Components/DebugCameraController.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Settings/EngineSettings.h"
#include "InputHandler.h"
#include "Maths.hpp"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr const char* DebugCameraActorName = "__DebugCamera";
	constexpr float MaximumPitchDegrees = 89.0f;
}

const DebugCameraPreset& GetDebugCameraPreset()
{
	static const DebugCameraPreset preset;
	return preset;
}

CameraComponent* DebugCameraService::Ensure(World& world, CommonUtilities::Vector2u clientSize)
{
	if (Actor* existingActor = world.FindActor(DebugCameraActorName))
	{
		if (CameraComponent* existingCamera = existingActor->GetComponent<CameraComponent>())
		{
			return existingCamera;
		}
	}

	Actor* actor = world.SpawnActor(DebugCameraActorName);

	const DebugCameraPreset& preset = GetDebugCameraPreset();
	actor->GetTransform().SetLocalPosition(preset.Position);
	actor->GetTransform().SetLocalRotation(preset.Rotation);

	CameraComponent* camera = actor->AddComponent<CameraComponent>(
		"Camera", preset.FieldOfView, preset.NearPlane, preset.FarPlane, clientSize);

	actor->AddComponent<DebugCameraController>("Controls");

	return camera;
}

void DebugCameraService::Toggle(World& world, CommonUtilities::Vector2u clientSize)
{
	CameraComponent* activeCamera = world.GetActiveCamera();
	if (activeCamera && activeCamera->GetOwner()->GetName() == DebugCameraActorName)
	{
		Actor* previousActor = world.FindActor(myPreviousActor);
		CameraComponent* previousCamera = previousActor
			? dynamic_cast<CameraComponent*>(previousActor->FindComponent(myPreviousComponent))
			: nullptr;
		if (previousCamera)
		{
			world.SetActiveCamera(previousCamera);
		}
		return;
	}
	if (activeCamera)
	{
		myPreviousActor = activeCamera->GetOwner()->GetName();
		myPreviousComponent = activeCamera->GetName();
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
	const CU::Vector3f forward = CU::NormalizeSafe(GetOwner()->GetTransform().GetLocalForward(), CU::Vector3f::UnitZ);
	myYaw = std::atan2(forward.x, forward.z);
	myPitch = -std::asin(CU::Clamp(forward.y, -1.f, 1.f));
	CommonUtilities::InputMapper& input = ServiceLocator::GetInstance().GetInputMapper();

	auto movementInput = [this, &input](std::string_view action, float& target, float aMultiplier)
	{
		myListenerIDs.push_back(input.AddEventListener(action, [&target, aMultiplier](const CommonUtilities::InputEvent& event)
		{
			target = static_cast<float>(event.inputData.isHeld) * aMultiplier;
		}));
	};
	movementInput("CameraForward", myInput.z, 1);
	movementInput("CameraBack", myInput.z, -1);
	movementInput("CameraRight", myInput.x, 1);
	movementInput("CameraLeft", myInput.x, -1);
	movementInput("CameraUp", myInput.y, 1);
	movementInput("CameraDown", myInput.y, -1);

	myListenerIDs.push_back(input.AddEventListener("CameraLookEnable", [this](const CommonUtilities::InputEvent& event)
		{
			myLook = event.inputData.isHeld;
			if (event.inputData.isPressed || event.inputData.isReleased)
			{
				ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->SetCursorVisibility(event.inputData.isReleased);
			}
		}));

	myListenerIDs.push_back(input.AddEventListener("CameraLookDelta", [this](const CommonUtilities::InputEvent& event)
	{
		if (event.isAxis2D)
		{
			myLookDelta += CommonUtilities::Vector2f{event.inputData.valueA, event.inputData.valueB};
		}
	}));
}

void DebugCameraController::Update(float deltaTime)
{
	if (!GetWorld().GetActiveCamera() || GetWorld().GetActiveCamera()->GetOwner() != GetOwner())
	{
		myLookDelta = {};
		return;
	}

	Transform& transform = GetOwner()->GetTransform();
	const DebugCameraPreset& preset = GetDebugCameraPreset();

	if (myLook)
	{
		myYaw += myLookDelta.x * preset.LookSensitivity;
		myPitch = CU::Clamp(myPitch + myLookDelta.y * preset.LookSensitivity,
		                    CU::DegreesToRadians(-MaximumPitchDegrees), CU::DegreesToRadians(MaximumPitchDegrees));

		CommonUtilities::Quaternion<float> yawRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CU::Vector3f::UnitY, myYaw);

		CommonUtilities::Quaternion<float> pitchRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CU::Vector3f::UnitX, myPitch);

		transform.SetLocalRotation(yawRotation * pitchRotation);

		CommonUtilities::InputHandler* inputHandler = ServiceLocator::GetInstance().GetInputMapper().GetInputHandler();
		if (ServiceLocator::GetInstance().GetEngineSettings().GetApplicationSettings().EnableMouseLook
			&& inputHandler && GetForegroundWindow() == inputHandler->GetWindowHandle())
		{
			inputHandler->CenterMouse();
		}
	}

	myLookDelta = {};

	CommonUtilities::Vector3f motion = (transform.GetLocalForward() * myInput.z) + (transform.GetLocalRight() * myInput.x);
	motion.y += myInput.y;
	if (motion.LengthSqr() > 0)
	{
		transform.SetLocalPosition(transform.GetLocalPosition() + CU::NormalizeSafe(motion) * preset.MoveSpeed * deltaTime);
	}
}

void DebugCameraController::EndPlay() noexcept
{
	CommonUtilities::InputMapper& input = ServiceLocator::GetInstance().GetInputMapper();
	for (unsigned id : myListenerIDs)
	{
		input.RemoveEventListener(id);
	}
	myListenerIDs.clear();
}
