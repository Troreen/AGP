#pragma once
#include "Ability.h"

class MeleeAbility final : public Ability
{
public:
	MeleeAbility() = default;

	bool CanActivate() override;
	bool TryActivate(const AbilityActivationContext& aContext) override;

private:
};

