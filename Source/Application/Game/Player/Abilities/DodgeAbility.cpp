#include "DodgeAbility.h"
#include <Player/Components/PlayerMovementComponent.h>


DodgeAbility::DodgeAbility(PlayerMovementComponent& aMovement) : myMovement(aMovement)
{

}

bool DodgeAbility::CanActivate()
{
	return myCooldownRemaining <= 0.0f;
}

bool DodgeAbility::TryActivate(const AbilityActivationContext& aContext)
{
	if (!CanActivate())
	{
		return false;
	}

	myDistance = 100.0f * 5.0f;

	if (!myMovement.StartDodge(aContext.Direction, myDistance, 1600.0f))
	{
		return false;
	}

	myCooldownRemaining = myCooldown;

	return true;
}
