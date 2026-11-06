#pragma once
#include "Ability.h"
#include <Vector3.hpp>

class PlayerMovementComponent;

class DashAbility final : public Ability
{
public:
	DashAbility() = delete;
	DashAbility(PlayerMovementComponent& aMovement);

	bool CanActivate() override;
	bool TryActivate(const AbilityActivationContext& aContext) override;

private:

	float myDistance = 0.0f;

	PlayerMovementComponent& myMovement;

};

