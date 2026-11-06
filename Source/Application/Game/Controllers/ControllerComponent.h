#pragma once
#include <GameFramework/Components/CustomComponent.h>
#include <CommonUtilities/Vector.hpp>
#include "Controller.h"
#include <unordered_map>
#include <memory>

class Transform;

struct ComponentControllerData
{
	std::vector<CommonUtilities::Vector3f> Path;
	int PathIndex = 0;
	float MinPathDistance;
	float MaxPlayerDistanceFromPath;
	CommonUtilities::Vector3f OwnerPos;
	CommonUtilities::Vector3f Velocity;

	float Speed;
	float Steering;
};

struct ControllerData
{
	std::shared_ptr<Controller> Controller;
	float Weight = 1;
};

class ControllerComponent : public CustomComponent
{
public:
	void InitializeCustomVariables(const CustomComponentVariables& someVariables) override;
	void BeginPlay() override;

	void Update(float aDeltaTime) override;
	void UpdateData();
	void UpdateMovement(float aDeltaTime);

	void AddController(const std::string& aName, std::shared_ptr<Controller> aController, float aWeight = 1.f);
	void SetWeight(const std::string& aName, float aWeight);

	void Limit(CommonUtilities::Vector3f& outVector, float aLimit);

	ComponentControllerData& GetData() { return myData; }
	const CommonUtilities::Vector3f& GetVelocity() { return myVelocity; }

private:
	float myMass;
	float myMaxForce;
	float myMaxSpeed;
	bool myActive;

	CommonUtilities::Vector3f mySterringForce;
	CommonUtilities::Vector3f myAcceleration;
	CommonUtilities::Vector3f myVelocity;

	Transform* myTransform;
	ComponentControllerData myData;

	std::unordered_map<std::string, ControllerData> myControllers;
};
