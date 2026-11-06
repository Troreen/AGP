#include "GameFramework.pch.h"
#include "DamageableComponent.h"
#include <GameFramework/Scenes/CustomComponentVariables.h>


void DamageableComponent::BeginPlay()
{
	myMaxHealth = 100;
	myCurrentHealth = myMaxHealth;
	myInvulnerabilityDuration = 1.0f;
}

void DamageableComponent::Update(float aDeltaTime)
{
	aDeltaTime;
	myHealthAtStartOfFrame = myCurrentHealth;

	if (myInvulnerabilityTimer > 0.0f)
	{
		myInvulnerabilityTimer = (std::max)(0.0f, myInvulnerabilityTimer - aDeltaTime);
	}
}

void DamageableComponent::ApplyDamage(int anAmount)
{
	if (IsInvicible() || anAmount <= 0)
	{
		return;
	}

	myCurrentHealth = std::clamp(myCurrentHealth - anAmount, 0, myMaxHealth);
}

//void DamageableComponent::InitializeCustomVariables(const CustomComponentVariables& someVariables)
//{
//	someVariables;
//	myMaxHealth = someVariables.GetOrDefault<int>("MaxHealth", 100);
//	myCurrentHealth = myMaxHealth;
//
//	myInvulnerabilityDuration = someVariables.GetOrDefault("InvulnerableDuration", 1.0f);
//}

int DamageableComponent::GetCurrentHealth() const
{
	return myCurrentHealth;
}

void DamageableComponent::SetCurrentHealth(int anAmount)
{
	if (anAmount <= 0)
	{
		return;
	}

	myCurrentHealth = anAmount;
}

void DamageableComponent::Heal(int anAmount)
{
	if (anAmount <= 0)
	{
		return;
	}

	SetCurrentHealth(myCurrentHealth + anAmount);
}


bool DamageableComponent::IsDead() const
{
	return myCurrentHealth <= 0;
}

bool DamageableComponent::TookDamageThisFrame() const
{
	return myCurrentHealth < myHealthAtStartOfFrame;
}

bool DamageableComponent::IsInvicible() const
{
	return myInvulnerabilityTimer > 0.0f;
}
