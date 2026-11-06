#include "AbilityComponent.h"
#include <GameFramework/World/Actor.h>
#include <Player/Abilities/DashAbility.h>
#include <Player/Abilities/DodgeAbility.h>
#include "PlayerMovementComponent.h"
#include <iostream>

AbilityComponent::~AbilityComponent()
{
	for (int i = 0; i < static_cast<int>(AbilitySlot::COUNT); i++)
	{
		delete mySlots[i];
		mySlots[i] = nullptr;
	}
}

void AbilityComponent::InitializeCustomVariables(const CustomComponentVariables& someVariables)
{
	someVariables;
}

void AbilityComponent::BeginPlay()
{
	myMovement = GetOwner()->GetComponent<PlayerMovementComponent>();
	if (myMovement)
	{
		myDashAbility = new DashAbility(*myMovement);

		AssignAbility(AbilitySlot::Slot2, myDashAbility);
	}
	if (myMovement)
	{
		myDodgeAbility = new DodgeAbility(*myMovement);

		AssignAbility(AbilitySlot::Space, myDodgeAbility);
	}
}

void AbilityComponent::AssignAbility(const AbilitySlot& aSlot, Ability* aAbility)
{
	mySlots[static_cast<int>(aSlot)] = aAbility;
}

bool AbilityComponent::TryActivateAbility(const AbilitySlot& aSlot, const AbilityActivationContext& aContext) const
{
	Ability* ability = mySlots[static_cast<int>(aSlot)];

	if (ability)
	{
		return ability->TryActivate(aContext);
	}
	return false;
}

void AbilityComponent::Update(float aDeltaTime)
{
	for (int i = 0; i < static_cast<int>(AbilitySlot::COUNT); i++)
	{
		if (mySlots[i] != nullptr)
		{
			mySlots[i]->Update(aDeltaTime);
		}
	}
}
