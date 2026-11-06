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
	void Update(float aDeltaTime) override;

private:

	Ability* mySlots[static_cast<int>(AbilitySlot::COUNT)] = {};

	Ability* myDashAbility = nullptr;
	Ability* myDodgeAbility = nullptr;

	PlayerMovementComponent* myMovement = nullptr;
};

