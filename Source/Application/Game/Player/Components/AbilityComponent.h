#pragma once
#include "GameFramework\Components\CustomComponent.h"

enum class AbilitySlot
{
	Slot1,
	Slot2,
	Slot3,
	Slot4,
	LeftMouse,
	RightMouse,
	Space,
	COUNT
};

class Ability;
class PlayerMovementComponent;

struct AbilityActivationContext;

class AbilityComponent : public CustomComponent
{
public:

	AbilityComponent() = default;
	~AbilityComponent() override;

	void InitializeCustomVariables(const CustomComponentVariables& someVariables) override;

	void BeginPlay() override;
	void AssignAbility(const AbilitySlot& aSlot, Ability* aAbility);
	bool TryActivateAbility(const AbilitySlot& aSlot, const AbilityActivationContext& aContext) const;
	void SetMeleeAttackHeld(bool aHeld);
	void Update(float aDeltaTime) override;

private:

	Ability* mySlots[static_cast<int>(AbilitySlot::COUNT)] = {};

	Ability* myMeleeAbility = nullptr;
	Ability* myHeavyMeleeAbility = nullptr;
	Ability* myAbsorptionAbility = nullptr;
	Ability* myDashAbility = nullptr;
	Ability* myPulseAbility = nullptr;
	Ability* myArrowStormAbility = nullptr;
	Ability* myDodgeAbility = nullptr;

	PlayerMovementComponent* myMovement = nullptr;
};

