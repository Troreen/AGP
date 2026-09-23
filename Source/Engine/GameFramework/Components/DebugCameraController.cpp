#include "GameFramework/ServiceLocator.h"
#include "GameFramework/Components/DebugCameraController.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/CameraComponent.h"
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
	actor->GetTransform().SetData(preset.Transform);
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
	CommonUtilities::InputMapper& input = *ServiceLocator::GetInstance().GetInputMapper();
	input.BindActionToInputCode("CameraLookEnable", EKeyCode::MOUSERBUTTON);
	input.BindActionToInputCode("CameraForward", EKeyCode::W);
	input.BindActionToInputCode("CameraBack", EKeyCode::S);
	input.BindActionToInputCode("CameraLeft", EKeyCode::A);
	input.BindActionToInputCode("CameraRight", EKeyCode::D);
	input.BindActionToInputCode("CameraUp", EKeyCode::SPACE);
	input.BindActionToInputCode("CameraDown", EKeyCode::CONTROL);
	input.BindActionToInputCode("CameraLookDelta", EPointerCode::MOUSE_DELTA);

	auto held = [this, &input](std::string_view action, bool& target)
	{
		myListenerIDs.push_back(input.AddEventListener(action, [&target](const CommonUtilities::InputEvent& event)
		{
			target = event.inputData.isHeld;
		}));
	};
	held("CameraLookEnable", myLook);
	held("CameraForward", myForward);
	held("CameraBack", myBack);
	held("CameraLeft", myLeft);
	held("CameraRight", myRight);
	held("CameraUp", myUp);
	held("CameraDown", myDown);
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
		transform.SetLocalRotationDegrees(CU::RadiansToDegrees(myYaw), CU::RadiansToDegrees(myPitch), 0);
	}
	myLookDelta = {};
	CommonUtilities::Vector3f motion{};
	if (myForward)
	{
		motion += CU::NormalizeSafe(transform.GetLocalForward());
	}
	if (myBack)
	{
		motion -= CU::NormalizeSafe(transform.GetLocalForward());
	}
	if (myRight)
	{
		motion += CU::NormalizeSafe(transform.GetLocalRight());
	}
	if (myLeft)
	{
		motion -= CU::NormalizeSafe(transform.GetLocalRight());
	}
	if (myUp)
	{
		motion += CommonUtilities::Vector3f::UnitY;
	}
	if (myDown)
	{
		motion -= CommonUtilities::Vector3f::UnitY;
	}
	if (motion.LengthSqr() > 0)
	{
		transform.SetLocalPosition(transform.GetLocalPosition() + CU::NormalizeSafe(motion) * preset.MoveSpeed * deltaTime);
	}
}

void DebugCameraController::EndPlay() noexcept
{
	auto* input = ServiceLocator::GetInstance().GetInputMapper();
	if (!input) return;
	for (unsigned id : myListenerIDs) input->RemoveEventListener(id);
	myListenerIDs.clear();
}
