#pragma once
#include "Ability.h"

class AbsorptionAbility : public Ability
{
public:

	AbsorptionAbility() = default;

	bool CanActivate() override;
	bool TryActivate(const AbilityActivationContext& aContext) override;

private:

};

