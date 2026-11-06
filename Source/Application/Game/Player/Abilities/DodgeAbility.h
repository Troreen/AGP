#pragma once
#include "Ability.h"


class PlayerMovementComponent;

class DodgeAbility : public Ability
{
public:
	DodgeAbility() = delete;
	DodgeAbility(PlayerMovementComponent& aMovement);

	bool CanActivate() override;
	bool TryActivate(const AbilityActivationContext& aContext) override;

private:

	float myDistance = 0.0f;

	PlayerMovementComponent& myMovement;
};

