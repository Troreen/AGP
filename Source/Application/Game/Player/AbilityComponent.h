#pragma once
#include "GameFramework\World\Component.h"

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

class AbilityComponent : public Component
{
public:

	AbilityComponent() = default;
	~AbilityComponent() override;

	void AssignAbility(const AbilitySlot& aSlot, Ability* aAbility);
	void TryActivateAbility(const AbilitySlot& aSlot);
	void Update(float aDeltaTime) override;

private:

	Ability* mySlots[static_cast<int>(AbilitySlot::COUNT)] = {};
};

