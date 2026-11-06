#include "PlayerMovementComponent.h"
#include <GameFramework/World/Actor.h>
#include <GameFramework/Components/AnimatorComponent.h>
#include <GameFramework/Scenes/CustomComponentVariables.h>
#include <GameFramework/ServiceLocator.h>
#include <Maths.hpp>
#include <GraphicsEngine/Drawers/LineDrawer.h>
#include <iostream>

PlayerMovementComponent::PlayerMovementComponent()
{
	myMaxSpeed = 300.0f;
	myTurnSpeed = 0.0f;
	myHasDestination = false;
	myArrivedDistance = 0.0f;
	myDashDistance = 0;
	myDashDuration = 0;
	myDashSpeed = 0;
	myDashTimer = 0;
	myMovementState = MovementState::None;
}

void PlayerMovementComponent::InitializeCustomVariables(const CustomComponentVariables& someVariables)
{
	myMaxSpeed = someVariables.GetOrDefault<float>("Speed", 300.0f);
}

void PlayerMovementComponent::BeginPlay()
{
	myVelocity = { 0.0f, 0.0f, 0.0f };
	myHasDestination = false;
	myArrivedDistance = 10.0f;
	myTurnSpeed = 10.0f;
	myMovementState = MovementState::None;
}

void PlayerMovementComponent::Update(float aDeltaTime)
{
	if (myScriptedMovement || myMovementState == MovementState::None)
	{
		return;
	}


	switch (myMovementState)
	{
		case MovementState::Moving:
			UpdateNormalMovement(aDeltaTime);
			return;
		case MovementState::Dashing:
			UpdateDash(aDeltaTime);
			return;
		case MovementState::Dodging:
			UpdateDodge(aDeltaTime);
			return;
		default:
			std::cout << "You really shouldnt be here... - movement\n";
			return;
	}
}

void PlayerMovementComponent::SetDestination(CommonUtilities::Vector3f aDestination)
{
	if (myScriptedMovement || !CanTakeMovementInput())
	{
		return;
	}

	myDestination = aDestination;
	myHasDestination = true;
	myMovementState = MovementState::Moving;
}

void PlayerMovementComponent::SetWaypoints(std::vector<CommonUtilities::Vector3f>&& someWaypoints)
{
	if (myScriptedMovement || !CanTakeMovementInput() || someWaypoints.empty())
	{
		return;
	}

	myWaypoints = std::move(someWaypoints);

	myDestination = myWaypoints[myWaypoints.size() - 2];

	myHasDestination = true;
	myMovementState = MovementState::Moving;

	if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
	{
		animation->GetTree().SetBool("IsRunning", true);
	}
}

void PlayerMovementComponent::StopMovement()
{
	myMovementState = MovementState::None;
	myDestination = { 0.0f, 0.0f, 0.0f };
	myVelocity = { 0.0f, 0.0f, 0.0f };
	myWaypoints.clear();
	myHasDestination = false;

	if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
	{
		animation->GetTree().SetBool("IsRunning", false);
	}
}

void PlayerMovementComponent::SetAbilityMovementLocked(bool aLocked)
{
	myAbilityMovementLocked = aLocked;

	if (aLocked)
	{
		StopMovement();
	}
}

bool PlayerMovementComponent::IsAbilityMovementLocked() const
{
	return myAbilityMovementLocked;
}

void PlayerMovementComponent::SetScriptedMovement(bool aScripted)
{
	myScriptedMovement = aScripted;
	if (aScripted)
	{
		StopMovement();
	}
}

void PlayerMovementComponent::RotateTowardsTarget(CommonUtilities::Vector3f aTarget, float aDeltaTime)
{
	auto& transform = GetOwner()->GetTransform();

	CommonUtilities::Vector3f direction = (aTarget - transform.GetLocalPosition()).GetNormalized();

	direction.y = 0.0f;

	if (direction.LengthSqr() < 0.0001f)
	{
		return;
	}

	direction.Normalize();

	float yawAngleRadians = std::atan2(direction.x, direction.z);

	float t = 1.0f - std::exp(-myTurnSpeed * aDeltaTime);
	t = CommonUtilities::Math::Clamp(t, 0.0f, 1.0f);

	CommonUtilities::Quaternion<float> turnRotation = CommonUtilities::Quaternion<float>::CreateFromAxisAngle(CommonUtilities::Vector3f::UnitY, yawAngleRadians);

	const CommonUtilities::Quaternion<float>& oldRotation = transform.GetLocalRotation();

	CommonUtilities::Quaternion<float> newRotation = CommonUtilities::Quaternion<float>::Slerp(oldRotation, turnRotation, t);

	transform.SetLocalRotation(newRotation);
}

void PlayerMovementComponent::RotateTowardsDirection(CommonUtilities::Vector3f aDirection, float aDeltaTime)
{
	aDeltaTime;

	auto& transform = GetOwner()->GetTransform();

	CommonUtilities::Vector3f direction = aDirection;

	direction.y = 0.0f;

	float yawAngleRadians = std::atan2(direction.x, direction.z);

	transform.SetLocalRotation({ 0.0f, yawAngleRadians, 0.0f });
}

void PlayerMovementComponent::UpdateNormalMovement(float aDeltaTime)
{
	CU::Vector3f position = GetOwner()->GetTransform().GetLocalPosition();

	if (myWaypoints.size() >= 2)
	{
		CU::Vector3f edge = myDestination - myWaypoints.back();

		CU::Vector3f translation = myDestination - position;

		if (edge.Dot(translation) <= 0)
		{
			myWaypoints.pop_back();
			if (myWaypoints.size() >= 2)
			{
				myDestination = myWaypoints[myWaypoints.size() - 2];
			}
			else
			{
				myWaypoints.clear();
				if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
				{
					animation->GetTree().SetBool("IsRunning", false);
				}
				myVelocity = { 0, 0, 0 };
				myHasDestination = false;
				myMovementState = MovementState::None;
			}
		}
	}

	CU::Vector3f translation = myDestination - position;

	//float distance = translation.Length();

	//if (distance <= myArrivedDistance)
	//{
	//	if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
	//	{
	//		animation->GetTree().SetBool("IsRunning", false);
	//	}
	//	myVelocity = { 0, 0, 0 };
	//	myHasDestination = false;
	//	myMovementState = MovementState::None;
	//	return;

	//}

	RotateTowardsTarget(myDestination, aDeltaTime);

	LineDrawer& lineDrawer = LineDrawer::Get();
	lineDrawer.DrawLine(position, myDestination, { 0.0f, 1.0f, 0.0f, 1.0f });

	CU::Vector3f direction = translation.GetNormalized();

	myVelocity = direction * myMaxSpeed;

	position += myVelocity * aDeltaTime;

	GetOwner()->GetTransform().SetLocalPosition(position);
}

void PlayerMovementComponent::UpdateDash(float aDeltaTime)
{
	CommonUtilities::Vector3f position = GetOwner()->GetTransform().GetLocalPosition();

	position += myVelocity * aDeltaTime;

	CommonUtilities::Vector3f target = myDashStartPosition + (myDashDirection * myDashDistance);

	LineDrawer& lineDrawer = LineDrawer::Get();
	lineDrawer.DrawLine(position, target, { 1.0f, 0.0f, 0.0f, 1.0f });

	GetOwner()->GetTransform().SetLocalPosition(position);

	RotateTowardsDirection(myDashDirection, aDeltaTime);

	myDashTimer += aDeltaTime;

	if (myDashTimer >= myDashDuration)
	{
		myDashTimer = 0.0f;
		StopMovement();
	}
}

void PlayerMovementComponent::UpdateDodge(float aDeltaTime)
{
	CommonUtilities::Vector3f position = GetOwner()->GetTransform().GetLocalPosition();

	position += myVelocity * aDeltaTime;

	CommonUtilities::Vector3f target = myDashStartPosition + (myDashDirection * myDashDistance);

	LineDrawer& lineDrawer = LineDrawer::Get();
	lineDrawer.DrawLine(position, target, { 0.0f, 0.0f, 1.0f, 1.0f });

	GetOwner()->GetTransform().SetLocalPosition(position);

	RotateTowardsDirection(myDashDirection, aDeltaTime);

	myDashTimer += aDeltaTime;

	if (myDashTimer >= myDashDuration)
	{
		myDashTimer = 0.0f;
		StopMovement();
	}
}

bool PlayerMovementComponent::CanTakeMovementInput() const
{
	return !myAbilityMovementLocked && myMovementState != MovementState::Dodging && myMovementState != MovementState::Dashing;
}

bool PlayerMovementComponent::IsMoving() const
{
	return myMovementState != MovementState::None;
}

bool PlayerMovementComponent::StartDash(const CommonUtilities::Vector3f& aDirection, float aDistance, float aSpeed)
{
	if (!CanTakeMovementInput())
	{
		return false;
	}

	if (aSpeed <= 0.0f)
	{
		return false;
	}

	myDashDistance = aDistance;
	myDashDirection = aDirection;
	myDashSpeed = aSpeed;

	myDashDuration = myDashDistance / myDashSpeed;
	myDashTimer = 0.0f;
	myVelocity = aDirection.GetNormalized() * myDashSpeed;

	myDashStartPosition = GetOwner()->GetTransform().GetLocalPosition();

	myMovementState = MovementState::Dashing;

	if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
	{
		animation->GetTree().SetTrigger("IsKB2");
	}

	return true;
}

bool PlayerMovementComponent::StartDodge(const CommonUtilities::Vector3f& aDirection, float aDistance, float aSpeed)
{
	if (!CanTakeMovementInput())
	{
		return false;
	}

	if (aSpeed <= 0.0f)
	{
		return false;
	}

	myDashDistance = aDistance;
	myDashDirection = aDirection;
	myDashSpeed = aSpeed;

	myDashDuration = myDashDistance / myDashSpeed;
	myDashTimer = 0.0f;
	myVelocity = aDirection.GetNormalized() * myDashSpeed;

	myDashStartPosition = GetOwner()->GetTransform().GetLocalPosition();

	myMovementState = MovementState::Dodging;

	if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
	{
		animation->GetTree().SetTrigger("IsPlayerDodging");
	}

	return true;
}
