#include "ControllerComponent.h"
#include <Application/Game/BehaviourTree/GenericBehaviourTree/BlackBoard.h>
#include <GameFramework/Scenes/CustomComponentVariables.h>
#include <GameFramework/ServiceLocator.h>
#include <GameFramework/World/Actor.h>
#include "ControllerFactory.h"
#include <CommonUtilities/Maths.hpp>

#define MIN_C_WEIGHT 0.0001f

void ControllerComponent::InitializeCustomVariables(const CustomComponentVariables& someVariables)
{
	for (auto& name : someVariables.StringVariables)
	{
		if (name.first == "Controller")
		{
			AddController(name.second, ControllerFactory::GetInstance().GetController(name.second));
		}
	}

	for (auto& weight : someVariables.FloatVariables)
	{
		if (myControllers.contains(weight.first))
		{
			myControllers[weight.first].Weight = weight.second;
		}
	}
}

void ControllerComponent::BeginPlay()
{
	myTransform = &GetOwner()->GetTransform();
	myMass = 1;
	myMaxForce = 1000;
	myMaxSpeed = 300;
	myActive = false;

	myData.MaxPlayerDistanceFromPath = 100.f;
	myData.MinPathDistance = 50.f;
	myData.Speed = 300.f;
	myData.Steering = 600;
	myData.PathIndex = -1;
}

void ControllerComponent::Update(float aDeltaTime)
{
	UpdateData();
	UpdateMovement(aDeltaTime);
}

void ControllerComponent::UpdateData()
{
	myData.OwnerPos = GetOwner()->GetTransform().GetWorldPosition();

	for (auto& controllerData : myControllers)
	{
		ControllerData& data = controllerData.second;

		if (data.Weight < MIN_C_WEIGHT)
		{
			continue;
		}

		Controller* controller = data.Controller.get();
		controller->UpdateActorControllerData(myData);
		myActive = true;
	}

	for (auto& controllerData : myControllers)
	{
		ControllerData& data = controllerData.second;

		if (data.Weight < MIN_C_WEIGHT)
		{
			continue;
		}

		Controller* controller = data.Controller.get();

		CommonUtilities::Vector3f force = controller->GetSteeringForce(this);
		float weight = data.Weight;

		CommonUtilities::Vector3f steeringForce = force * weight;

		mySterringForce += steeringForce;
	}
}

void ControllerComponent::UpdateMovement(float aDeltaTime)
{
	if (!myActive)
	{
		return;
	}
	Limit(mySterringForce, myMaxForce);
	myAcceleration = mySterringForce * myMass;

	myVelocity += myAcceleration * aDeltaTime;
	Limit(myVelocity, myMaxSpeed);

	float rotation = CommonUtilities::RadiansToDegrees(atan2f(myVelocity.x, myVelocity.z));

	myTransform->SetWorldPosition(myTransform->GetWorldPosition() + myVelocity * aDeltaTime);
	myTransform->SetLocalRotationDegrees({rotation, 0, 0});
	myActive = false;
}

void ControllerComponent::AddController(const std::string& aName, std::shared_ptr<Controller> aController, float aWeight)
{
	myControllers.emplace(aName, ControllerData{ aController, aWeight });
}

void ControllerComponent::SetWeight(const std::string& aName, float aWeight)
{
	myControllers[aName].Weight = aWeight;
}

void ControllerComponent::Limit(CommonUtilities::Vector3f& outVector, float aLimit)
{
	if (outVector.LengthSqr() > aLimit * aLimit)
	{
		outVector = outVector.GetNormalized() * aLimit;
	}
}
