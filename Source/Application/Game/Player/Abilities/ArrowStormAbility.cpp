#include "ArrowStormAbility.h"

bool ArrowStormAbility::CanActivate()
{
    return false;
}

bool ArrowStormAbility::TryActivate(const AbilityActivationContext& aContext)
{
	aContext;
	return false;
}
