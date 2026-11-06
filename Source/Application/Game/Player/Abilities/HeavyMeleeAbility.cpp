#include "HeavyMeleeAbility.h"

bool HeavyMeleeAbility::CanActivate()
{
    return false;
}

bool HeavyMeleeAbility::TryActivate(const AbilityActivationContext& aContext)
{
	aContext;
	return false;
}
