#pragma once
#include "Ability.h"

class ArrowStormAbility : public Ability
{
public:

	ArrowStormAbility() = default;

	bool CanActivate() override;
	bool TryActivate(const AbilityActivationContext& aContext) override;

private:

};

