#pragma once
#include <Vector.hpp>
#include <vector>

class ControllerComponent;
struct ComponentControllerData;

class Controller
{
public:
	Controller() = default;
	~Controller() = default;

	virtual void UpdateActorControllerData(ComponentControllerData&) {};

	virtual const CommonUtilities::Vector3f GetDesiredVelocity(ControllerComponent* aComponent) const = 0;
	virtual const CommonUtilities::Vector3f GetSteeringForce(ControllerComponent* aComponent) const = 0;

private:
};
