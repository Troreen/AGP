#include "AbilityComponent.h"
#include "Ability.h"

AbilityComponent::~AbilityComponent()
{
	for (int i = 0; i < static_cast<int>(AbilitySlot::COUNT); i++)
	{
		delete mySlots[i];
		mySlots[i] = nullptr;
	}
}

void AbilityComponent::AssignAbility(const AbilitySlot& aSlot, Ability* aAbility)
{
	mySlots[static_cast<int>(aSlot)] = aAbility;
}

void AbilityComponent::TryActivateAbility(const AbilitySlot& aSlot)
{
	Ability* ability = mySlots[static_cast<int>(aSlot)];

	if (ability && ability->CanActivate())
	{
		ability->Activate();
	}
}

void AbilityComponent::Update(float aDeltaTime)
{
	for (int i = 0; i < static_cast<int>(AbilitySlot::COUNT); i++)
	{
		mySlots[i]->Update(aDeltaTime);
	}
}
