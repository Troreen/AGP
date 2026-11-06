#pragma once
#include "Ability.h"

class HeavyMeleeAbility : public Ability
{
public:

	HeavyMeleeAbility() = default;

	bool CanActivate() override;
	bool TryActivate(const AbilityActivationContext& aContext) override;

private:

};

