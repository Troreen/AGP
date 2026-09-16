#include "FreeFlyCameraController.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float LOC_DEFAULT_MOVE_SPEED = 500.f;
	constexpr float LOC_DEFAULT_LOOK_SENSITIVITY = 0.0025f;
	constexpr float LOC_DEFAULT_MAX_PITCH_RADIANS = 1.55334303f;
}

FreeFlyCameraController::FreeFlyCameraController()
	: myInputHandler(nullptr)
	, myTransform(nullptr)
	, myMoveSpeed(LOC_DEFAULT_MOVE_SPEED)
	, myLookSensitivity(LOC_DEFAULT_LOOK_SENSITIVITY)
	, myYawRadians(0.f)
	, myPitchRadians(0.f)
	, myMaxPitchRadians(LOC_DEFAULT_MAX_PITCH_RADIANS)
	, myHasMouseLookAnchor(false)
{
}

void FreeFlyCameraController::Init(CommonUtilities::InputHandler& anInputHandler, CommonUtilities::Transform& aTransform)
{
	myInputHandler = &anInputHandler;
	Init(aTransform);
}

void FreeFlyCameraController::Init(CommonUtilities::Transform& aTransform)
{
	myTransform = &aTransform;
	myHasMouseLookAnchor = false;

	const CommonUtilities::Vector3<float> startForward = myTransform->GetForward().GetNormalized();
	myYawRadians = std::atan2(startForward.x, startForward.z);
	myPitchRadians = -std::asin(std::clamp(startForward.y, -1.f, 1.f));

	CommonUtilities::Quaternion<float> yawRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3<float>::UnitY, myYawRadians);
	CommonUtilities::Quaternion<float> pitchRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3<float>::UnitX, myPitchRadians);
	CommonUtilities::Quaternion<float> cameraRotation = yawRotation * pitchRotation;
	cameraRotation.Normalize();
	myTransform->SetRotation(cameraRotation);
}

void FreeFlyCameraController::Update(float aTimeDelta)
{
	if (myInputHandler == nullptr || myTransform == nullptr)
	{
		return;
	}

	auto isVirtualKeyDown = [](int aVirtualKey)
	{
		return (GetAsyncKeyState(aVirtualKey) & 0x8000) != 0;
	};

	const HWND windowHandle = myInputHandler->GetWindowHandle();
	const bool isFocused = windowHandle != nullptr && GetForegroundWindow() == windowHandle;
	const bool rightMouseDown =
		myInputHandler->IsMouseButtonDown(Keys::MOUSERBUTTON) ||
		(isFocused && isVirtualKeyDown(static_cast<int>(Keys::MOUSERBUTTON)));

	if (isFocused && rightMouseDown)
	{
		RECT clientRect = {};
		if (GetClientRect(windowHandle, &clientRect) != 0)
		{
			const POINT centerPoint = {
				(clientRect.right - clientRect.left) / 2,
				(clientRect.bottom - clientRect.top) / 2
			};

			if (myHasMouseLookAnchor)
			{
				POINT mousePosScreen = {};
				GetCursorPos(&mousePosScreen);
				POINT mousePosClient = mousePosScreen;
				ScreenToClient(windowHandle, &mousePosClient);
				const float mouseDeltaX = static_cast<float>(mousePosClient.x - centerPoint.x);
				const float mouseDeltaY = static_cast<float>(mousePosClient.y - centerPoint.y);

				myYawRadians += mouseDeltaX * myLookSensitivity;
				myPitchRadians += mouseDeltaY * myLookSensitivity;
				myPitchRadians = std::clamp(myPitchRadians, -myMaxPitchRadians, myMaxPitchRadians);

				CommonUtilities::Quaternion<float> yawRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3<float>::UnitY, myYawRadians);
				CommonUtilities::Quaternion<float> pitchRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3<float>::UnitX, myPitchRadians);
				CommonUtilities::Quaternion<float> cameraRotation = yawRotation * pitchRotation;
				cameraRotation.Normalize();
				myTransform->SetRotation(cameraRotation);
			}

			POINT centerPointScreen = centerPoint;
			ClientToScreen(windowHandle, &centerPointScreen);
			SetCursorPos(centerPointScreen.x, centerPointScreen.y);
			myHasMouseLookAnchor = true;
		}
	}
	else
	{
		myHasMouseLookAnchor = false;
	}

	const bool wDown = myInputHandler->IsKeyDown(Keys::W) || (isFocused && isVirtualKeyDown(static_cast<int>(Keys::W)));
	const bool sDown = myInputHandler->IsKeyDown(Keys::S) || (isFocused && isVirtualKeyDown(static_cast<int>(Keys::S)));
	const bool dDown = myInputHandler->IsKeyDown(Keys::D) || (isFocused && isVirtualKeyDown(static_cast<int>(Keys::D)));
	const bool aDown = myInputHandler->IsKeyDown(Keys::A) || (isFocused && isVirtualKeyDown(static_cast<int>(Keys::A)));
	const bool spaceDown = myInputHandler->IsKeyDown(Keys::SPACE) || (isFocused && isVirtualKeyDown(static_cast<int>(Keys::SPACE)));
	const bool controlDown = myInputHandler->IsKeyDown(Keys::CONTROL) || (isFocused && isVirtualKeyDown(static_cast<int>(Keys::CONTROL)));

	InputState inputState;
	inputState.MoveForward = wDown;
	inputState.MoveBackward = sDown;
	inputState.MoveRight = dDown;
	inputState.MoveLeft = aDown;
	inputState.MoveUp = spaceDown;
	inputState.MoveDown = controlDown;
	Update(aTimeDelta, inputState);
}

void FreeFlyCameraController::Update(float aTimeDelta, const InputState& anInputState)
{
	if (myTransform == nullptr)
	{
		return;
	}

	if (anInputState.MouseLookActive)
	{
		myYawRadians += anInputState.MouseDeltaX * myLookSensitivity;
		myPitchRadians += anInputState.MouseDeltaY * myLookSensitivity;
		myPitchRadians = std::clamp(myPitchRadians, -myMaxPitchRadians, myMaxPitchRadians);

		CommonUtilities::Quaternion<float> yawRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3<float>::UnitY, myYawRadians);
		CommonUtilities::Quaternion<float> pitchRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3<float>::UnitX, myPitchRadians);
		CommonUtilities::Quaternion<float> cameraRotation = yawRotation * pitchRotation;
		cameraRotation.Normalize();
		myTransform->SetRotation(cameraRotation);
	}

	CommonUtilities::Vector3<float> cameraForward = myTransform->GetForward();
	if (cameraForward.LengthSqr() > 0.f)
	{
		cameraForward.Normalize();
	}
	else
	{
		cameraForward = CommonUtilities::Vector3<float>::UnitZ;
	}

	CommonUtilities::Vector3<float> cameraRight = myTransform->GetRight();
	if (cameraRight.LengthSqr() > 0.f)
	{
		cameraRight.Normalize();
	}
	else
	{
		cameraRight = CommonUtilities::Vector3<float>::UnitX;
	}

	CommonUtilities::Vector3<float> moveDirection = CommonUtilities::Vector3<float>::Zero;
	if (anInputState.MoveForward)
	{
		moveDirection += cameraForward;
	}
	if (anInputState.MoveBackward)
	{
		moveDirection -= cameraForward;
	}
	if (anInputState.MoveRight)
	{
		moveDirection += cameraRight;
	}
	if (anInputState.MoveLeft)
	{
		moveDirection -= cameraRight;
	}
	if (anInputState.MoveUp)
	{
		moveDirection += CommonUtilities::Vector3<float>::UnitY;
	}
	if (anInputState.MoveDown)
	{
		moveDirection -= CommonUtilities::Vector3<float>::UnitY;
	}

	if (moveDirection.LengthSqr() > 0.f)
	{
		moveDirection.Normalize();
		myTransform->SetPosition(myTransform->GetPosition() + moveDirection * (myMoveSpeed * aTimeDelta));
	}
}

void FreeFlyCameraController::ResetMouseLookAnchor()
{
	myHasMouseLookAnchor = false;
}

void FreeFlyCameraController::SetMoveSpeed(float aMoveSpeed)
{
	myMoveSpeed = std::clamp(aMoveSpeed, 1.0f, 50000.0f);
}

void FreeFlyCameraController::SetLookSensitivity(float aLookSensitivity)
{
	myLookSensitivity = std::clamp(aLookSensitivity, 0.0001f, 0.05f);
}

float FreeFlyCameraController::GetMoveSpeed() const
{
	return myMoveSpeed;
}

float FreeFlyCameraController::GetLookSensitivity() const
{
	return myLookSensitivity;
}
