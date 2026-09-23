#include "CameraControlsComponent.h"

#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/World/Actor.h"
#include "Maths.hpp"

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

	InputSystem& input = GetInputSystem();
	BindHeldInput(input, InputActions::CameraLookEnable, myLookActive);
	BindHeldInput(input, InputActions::CameraForward, myMoveForward);
	BindHeldInput(input, InputActions::CameraBack, myMoveBack);
	BindHeldInput(input, InputActions::CameraLeft, myMoveLeft);
	BindHeldInput(input, InputActions::CameraRight, myMoveRight);
	BindHeldInput(input, InputActions::CameraUp, myMoveUp);
	BindHeldInput(input, InputActions::CameraDown, myMoveDown);

	InputSubscription lookDeltaSubscription = input.Subscribe(
		InputActions::CameraLookDelta,
		[this](const InputActionEvent& event)
		{
			if (event.Phase == InputActionPhase::Ended)
			{
				return;
			}

			myLookDelta += std::get<Vector2f>(event.Value);
		});

	mySubscriptions.push_back(std::move(lookDeltaSubscription));
}

void CameraControlsComponent::Update(float deltaTime)
{
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

void CameraControlsComponent::BindHeldInput(InputSystem& aInput, const InputActionId& aAction, bool& aState)
{
	InputSubscription subscription = aInput.Subscribe(
		aAction,
		[&aState](const InputActionEvent& event)
		{
			aState = event.Phase != InputActionPhase::Ended;
		});

	mySubscriptions.push_back(std::move(subscription));
}
