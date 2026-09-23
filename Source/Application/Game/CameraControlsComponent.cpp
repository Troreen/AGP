#include "GameFramework/ServiceLocator.h"
#include "CameraControlsComponent.h"

#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/World/Actor.h"
#include "Maths.hpp"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/CameraComponent.h"

#include <cmath>
#include <utility>

namespace
{
	using Vector2f = CU::Vector2f;
	using Vector3f = CU::Vector3f;

	constexpr float CameraLookSensitivity = 0.0025f;
	constexpr float MaximumCameraPitchDegrees = 89.0f;
	constexpr float CameraMoveSpeed = 500.0f;
}

void CameraControlsComponent::BeginPlay()
{
	const Vector3f ownerForward = GetOwner()->GetTransform().GetLocalForward();
	const Vector3f direction = CU::NormalizeSafe(ownerForward, Vector3f::UnitZ);

	myYaw = std::atan2(direction.x, direction.z);
	myPitch = -std::asin(CU::Clamp(direction.y, -1.0f, 1.0f));

	CommonUtilities::InputMapper& input = *ServiceLocator::GetInstance().GetInputMapper();
	input.BindActionToInputCode("CameraLookEnable", EKeyCode::MOUSERBUTTON);
	input.BindActionToInputCode("CameraForward", EKeyCode::W);
	input.BindActionToInputCode("CameraBack", EKeyCode::S);
	input.BindActionToInputCode("CameraLeft", EKeyCode::A);
	input.BindActionToInputCode("CameraRight", EKeyCode::D);
	input.BindActionToInputCode("CameraUp", EKeyCode::SPACE);
	input.BindActionToInputCode("CameraDown", EKeyCode::CONTROL);
	input.BindActionToInputCode("CameraLookDelta", EPointerCode::MOUSE_DELTA);

	BindHeldInput(input, "CameraLookEnable", myLookActive);
	BindHeldInput(input, "CameraForward", myMoveForward);
	BindHeldInput(input, "CameraBack", myMoveBack);
	BindHeldInput(input, "CameraLeft", myMoveLeft);
	BindHeldInput(input, "CameraRight", myMoveRight);
	BindHeldInput(input, "CameraUp", myMoveUp);
	BindHeldInput(input, "CameraDown", myMoveDown);

	unsigned lookDeltaListenerID = input.AddEventListener(
		"CameraLookDelta",
		[this](const CommonUtilities::InputEvent& event)
		{
			if (event.inputData.isReleased)
			{
				return;
			}

			myLookDelta += CommonUtilities::Vector2f{event.inputData.valueA, event.inputData.valueB};
		});

	myListenerIDs.push_back(lookDeltaListenerID);
}

void CameraControlsComponent::Update(float deltaTime)
{
	if (GetWorld().GetActiveCamera() && GetWorld().GetActiveCamera()->GetOwner() != GetOwner())
	{
		myLookDelta = {};
		return;
	}
	Transform& transform = GetOwner()->GetTransform();

	if (myLookActive)
	{
		myYaw += myLookDelta.x * CameraLookSensitivity;
		myPitch += myLookDelta.y * CameraLookSensitivity;

		const float minimumPitch = CU::DegreesToRadians(-MaximumCameraPitchDegrees);
		const float maximumPitch = CU::DegreesToRadians(MaximumCameraPitchDegrees);
		myPitch = CU::Clamp(myPitch, minimumPitch, maximumPitch);

		const float yawDegrees = CU::RadiansToDegrees(myYaw);
		const float pitchDegrees = CU::RadiansToDegrees(myPitch);
		transform.SetLocalRotationDegrees(yawDegrees, pitchDegrees, 0.0f);
	}

	myLookDelta = {};

	Vector3f movement{};
	const Vector3f forward = CU::NormalizeSafe(transform.GetLocalForward());
	const Vector3f right = CU::NormalizeSafe(transform.GetLocalRight());

	if (myMoveForward) movement += forward;
	if (myMoveBack) movement -= forward;
	if (myMoveRight) movement += right;
	if (myMoveLeft) movement -= right;
	if (myMoveUp) movement += Vector3f::UnitY;
	if (myMoveDown) movement -= Vector3f::UnitY;
	if (movement.LengthSqr() == 0.0f)
	{
		return;
	}

	const Vector3f movementDirection = CU::NormalizeSafe(movement);
	const Vector3f movementThisFrame = movementDirection * CameraMoveSpeed * deltaTime;
	const Vector3f currentPosition = transform.GetLocalPosition();
	transform.SetLocalPosition(currentPosition + movementThisFrame);
}

void CameraControlsComponent::BindHeldInput(CommonUtilities::InputMapper& aInput, std::string_view aAction, bool& aState)
{
	unsigned listenerID = aInput.AddEventListener(
		aAction,
		[&aState](const CommonUtilities::InputEvent& event)
		{
			aState = event.inputData.isHeld;
		});

	myListenerIDs.push_back(listenerID);
}

void CameraControlsComponent::EndPlay() noexcept
{
	auto* input = ServiceLocator::GetInstance().GetInputMapper();
	if (!input) return;
	for (unsigned id : myListenerIDs) input->RemoveEventListener(id);
	myListenerIDs.clear();
}
