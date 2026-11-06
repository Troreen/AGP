#pragma once
#include "AbilityData.h"
#include <Vector3.hpp>

struct AbilityActivationContext
{
	CommonUtilities::Vector3f Direction;
};

class Ability
{
public:

	Ability() = default;
	virtual ~Ability();

	virtual bool CanActivate() = 0;
	virtual bool TryActivate(const AbilityActivationContext& aContext) = 0;
	virtual void Update(float aDeltaTime);

protected:

	float myCooldown = 0;
	float myCooldownRemaining = 0;

private:

};

