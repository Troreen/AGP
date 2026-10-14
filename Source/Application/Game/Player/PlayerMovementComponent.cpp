#include "PlayerMovementComponent.h"
#include <GameFramework/World/Actor.h>
#include <Maths.hpp>
#include <iostream>

PlayerMovementComponent::PlayerMovementComponent()
{
	myMaxSpeed = 0.f;
	myTurnSpeed = 0.0f;
	myHasDestination = false;
	myArrivedDistance = 0.0f;
}

void PlayerMovementComponent::BeginPlay()
{
	myMaxSpeed = 300.f;
	myVelocity = { 0.0f, 0.0f, 0.0f };
	myHasDestination = false;
	myArrivedDistance = 10.0f;
	myTurnSpeed = 100.0f;
}

void PlayerMovementComponent::Update(float aDeltaTime)
{
	if (!myHasDestination)
	{
		return;
	}

	CommonUtilities::Vector3f position = GetOwner()->GetTransform().GetLocalPosition();

	CommonUtilities::Vector3f direction = myDestination - position;
	float distance = (myDestination - position).Length();

	if (distance <= myArrivedDistance)
	{
		myDestination = { 0.0f, 0.0f, 0.0f };
		myHasDestination = false;
		return;
	}

	direction = direction.GetNormalized();

	myVelocity = direction * myMaxSpeed;

	position += myVelocity * aDeltaTime;

	GetOwner()->GetTransform().SetLocalPosition(position);

	RotateTowardsTarget(myDestination, aDeltaTime);

}

void PlayerMovementComponent::SetDestination(CommonUtilities::Vector3f aDestination)
{
	myDestination = aDestination;
	myHasDestination = true;
}

void PlayerMovementComponent::RotateTowardsTarget(CommonUtilities::Vector3f aTarget, float aDeltaTime)
{
	auto& transform = GetOwner()->GetTransform();

	CommonUtilities::Vector3f direction = aTarget - transform.GetLocalPosition();

	direction.y = 0.0f;

	float yawAngleRadians = std::atan2(direction.x, direction.z);

	float yawAngleDegrees = CommonUtilities::RadiansToDegrees(yawAngleRadians);

	transform.SetLocalRotationDegrees({ yawAngleDegrees, 0.0f, 0.0f });

	//float t = std::clamp(myTurnSpeed * aDeltaTime, 0.0f, 1.0f);

	//CommonUtilities::Quaternion<float> turnRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3f::UnitY, angle);

	//CommonUtilities::Vector3f rotationInDegrees = transform.GetLocalRotationDegrees();

	//CommonUtilities::Vector3f rotationInRadians =
	//{
	//	CommonUtilities::DegreesToRadians(rotationInDegrees.x),
	//	CommonUtilities::DegreesToRadians(rotationInDegrees.y),
	//	CommonUtilities::DegreesToRadians(rotationInDegrees.z)
	//};

	//CommonUtilities::Quaternion<float> oldRotation = CommonUtilities::Quaternion<float>::CreateFromYawPitchRoll(rotationInRadians.y, rotationInRadians.x, rotationInRadians.z);

	//CommonUtilities::Quaternion<float> newRotation = CommonUtilities::Quaternion<float>::Slerp(oldRotation, turnRotation, t);

	//CommonUtilities::Vector3f newRotationInRadians = newRotation.GetEulerAngles();

	//CommonUtilities::Vector3f newRotationInDegrees =
	//{
	//	CommonUtilities::RadiansToDegrees(newRotationInRadians.x),
	//	CommonUtilities::RadiansToDegrees(newRotationInRadians.y),
	//	CommonUtilities::RadiansToDegrees(newRotationInRadians.z)
	//};

	//transform.SetLocalRotationDegrees(newRotationInDegrees);

	//if (rotationInRadians != newRotationInRadians)
	//{
	//	std::cout << "Rotation: X: " << newRotationInRadians.x << " Y: " << newRotationInRadians.y << " Z: " << newRotationInRadians.z << "\n";
	//}
}
