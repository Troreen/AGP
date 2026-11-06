#include "DashAbility.h"
#include <Player/Components/PlayerMovementComponent.h>

DashAbility::DashAbility(PlayerMovementComponent& aMovement) : myMovement(aMovement)
{
	myCooldown = 0.5f;
}

bool DashAbility::CanActivate()
{
	return myCooldownRemaining <= 0.0f;
}

bool DashAbility::TryActivate(const AbilityActivationContext& aContext)
{
	if (!CanActivate())
	{
		return false;
	}

	myDistance = 100.0f * 8.0f;

	if (!myMovement.StartDash(aContext.Direction, myDistance, 3200.0f))
	{
		return false;
	}

	myCooldownRemaining = myCooldown;

	return true;
}
