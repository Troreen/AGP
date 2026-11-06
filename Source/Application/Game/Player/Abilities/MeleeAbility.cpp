#include "MeleeAbility.h"

bool MeleeAbility::CanActivate()
{
	return false;
}

bool MeleeAbility::TryActivate(const AbilityActivationContext& aContext)
{
	aContext;

	return false;
}
