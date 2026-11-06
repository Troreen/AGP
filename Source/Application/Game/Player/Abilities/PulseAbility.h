#pragma once
#include "Ability.h"

class PulseAbility : public Ability
{
public:

	PulseAbility() = default;

	bool CanActivate() override;
	bool TryActivate(const AbilityActivationContext& aContext) override;

private:

};

