#include "AbsorptionAbility.h"

bool AbsorptionAbility::CanActivate()
{
    return false;
}

bool AbsorptionAbility::TryActivate(const AbilityActivationContext& aContext)
{
	aContext;
	return false;
}
