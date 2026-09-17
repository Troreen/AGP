#include "FreeFlyCameraController.h"
#include "GameFramework/ServiceLocator.h"
#include <InputMapper.h>
#include <InputHandler.h>
#include <EnumKeyCode.h>
#include <EnumPointerCode.h>
#include <algorithm>

namespace
{
	constexpr float LOC_DEFAULT_MOVE_SPEED = 500.f;
	constexpr float LOC_DEFAULT_LOOK_SENSITIVITY = 0.0025f;
	constexpr float LOC_DEFAULT_MAX_PITCH_RADIANS = 1.55334303f;
}

FreeFlyCameraController::FreeFlyCameraController()
	: myTransform(nullptr)
	, myMoveSpeed(LOC_DEFAULT_MOVE_SPEED)
	, myLookSensitivity(LOC_DEFAULT_LOOK_SENSITIVITY)
	, myYawRadians(0.f)
	, myPitchRadians(0.f)
	, myMaxPitchRadians(LOC_DEFAULT_MAX_PITCH_RADIANS)
	, myHasMouseLookAnchor(false)
{
}

FreeFlyCameraController::~FreeFlyCameraController()
{
	CommonUtilities::InputMapper& inputMapper = *ServiceLocator::GetInstance().GetInputMapper();
	inputMapper.ClearBindingsFromAction("MOVE_FORWARD");
	inputMapper.ClearBindingsFromAction("MOVE_LEFT");
	inputMapper.ClearBindingsFromAction("MOVE_BACK");
	inputMapper.ClearBindingsFromAction("MOVE_RIGHT");
	inputMapper.ClearBindingsFromAction("MOVE_UP");
	inputMapper.ClearBindingsFromAction("MOVE_DOWN");
	inputMapper.ClearBindingsFromAction("MOUSE_LOCK");
	inputMapper.ClearBindingsFromAction("MOUSE_LOCK_MOVE");

	for (unsigned eventListenerID : myInputEventListenerIDs)
	{
		inputMapper.RemoveEventListener(eventListenerID);
	}
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

	CommonUtilities::InputMapper& inputMapper = *ServiceLocator::GetInstance().GetInputMapper();
	inputMapper.BindActionToInputCode("MOVE_FORWARD", EKeyCode::W);
	inputMapper.BindActionToInputCode("MOVE_LEFT", EKeyCode::A);
	inputMapper.BindActionToInputCode("MOVE_BACK", EKeyCode::S);
	inputMapper.BindActionToInputCode("MOVE_RIGHT", EKeyCode::D);
	inputMapper.BindActionToInputCode("MOVE_UP", EKeyCode::SPACE);
	inputMapper.BindActionToInputCode("MOVE_DOWN", EKeyCode::CONTROL);
	inputMapper.BindActionToInputCode("MOUSE_LOCK", EKeyCode::MOUSERBUTTON);
	inputMapper.BindActionToInputCode("MOUSE_LOCK_MOVE", EPointerCode::MOUSE_DELTA);

	myInputEventListenerIDs.push_back(
		inputMapper.AddEventListener("MOVE_FORWARD", [this](const CommonUtilities::InputEvent& anEvent) {
			myMoveBuffer += myTransform->GetForward() * anEvent.inputData.valueA;
		})
	);

	myInputEventListenerIDs.push_back(
		inputMapper.AddEventListener("MOVE_LEFT", [this](const CommonUtilities::InputEvent& anEvent) {
			myMoveBuffer -= myTransform->GetRight() * anEvent.inputData.valueA;
		})
	);

	myInputEventListenerIDs.push_back(
		inputMapper.AddEventListener("MOVE_BACK", [this](const CommonUtilities::InputEvent& anEvent) {
			myMoveBuffer -= myTransform->GetForward() * anEvent.inputData.valueA;
		})
	);

	myInputEventListenerIDs.push_back(
		inputMapper.AddEventListener("MOVE_RIGHT", [this](const CommonUtilities::InputEvent& anEvent) {
			myMoveBuffer += myTransform->GetRight() * anEvent.inputData.valueA;
		})
	);

	myInputEventListenerIDs.push_back(
		inputMapper.AddEventListener("MOVE_UP", [this](const CommonUtilities::InputEvent& anEvent) {
			myMoveBuffer += CommonUtilities::Vector3<float>::UnitY * anEvent.inputData.valueA;
		})
	);

	myInputEventListenerIDs.push_back(
		inputMapper.AddEventListener("MOVE_DOWN", [this](const CommonUtilities::InputEvent& anEvent) {
			myMoveBuffer -= CommonUtilities::Vector3<float>::UnitY * anEvent.inputData.valueA;
		})
	);

	myInputEventListenerIDs.push_back(
		inputMapper.AddEventListener("MOUSE_LOCK", [this](const CommonUtilities::InputEvent& anEvent) {
			myHasMouseLookAnchor = anEvent.inputData.isHeld;
		})
	);

	myInputEventListenerIDs.push_back(
		inputMapper.AddEventListener("MOUSE_LOCK_MOVE", [this](const CommonUtilities::InputEvent& anEvent) {
			if (myHasMouseLookAnchor)
			{
				HandleLockedMouseMove(anEvent);
			}
		})
	);
}

void FreeFlyCameraController::Update(float aTimeDelta)
{
	if (myTransform == nullptr)
	{
		return;
	}

	if (myMoveBuffer.LengthSqr() > 0.f)
	{
		myMoveBuffer.Normalize();
		myTransform->SetPosition(myTransform->GetPosition() + myMoveBuffer * (myMoveSpeed * aTimeDelta));
		myMoveBuffer.x = 0.f;
		myMoveBuffer.y = 0.f;
		myMoveBuffer.z = 0.f;
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

void FreeFlyCameraController::HandleLockedMouseMove(const CommonUtilities::InputEvent& anEvent)
{
	myYawRadians += anEvent.inputData.valueA * myLookSensitivity;
	myPitchRadians += anEvent.inputData.valueB * myLookSensitivity;
	myPitchRadians = std::clamp(myPitchRadians, -myMaxPitchRadians, myMaxPitchRadians);

	CommonUtilities::Quaternion<float> yawRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3<float>::UnitY, myYawRadians);
	CommonUtilities::Quaternion<float> pitchRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3<float>::UnitX, myPitchRadians);
	CommonUtilities::Quaternion<float> cameraRotation = yawRotation * pitchRotation;
	cameraRotation.Normalize();
	myTransform->SetRotation(cameraRotation);

	ServiceLocator::GetInstance().GetInputMapper()->GetInputHandler()->CenterMouse();
}
