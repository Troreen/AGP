#pragma once
#include <GameFramework/Components/CustomComponent.h>

class DamageableComponent : public Component
{
public:

	DamageableComponent() = default;

	//void InitializeCustomVariables(const CustomComponentVariables& someVariables) override;

	void BeginPlay() override;

	void Update(float aDeltaTime) override;

	void ApplyDamage(int anAmount);

	int GetCurrentHealth() const;
	void SetCurrentHealth(int anAmount);

	void Heal(int anAmount);

	bool IsDead() const;
	bool TookDamageThisFrame() const;
	bool IsInvicible() const;

private:

	int myMaxHealth;
	int myCurrentHealth;
	int myHealthAtStartOfFrame;

	bool myHasDied = false;

	float myInvulnerabilityDuration;
	float myInvulnerabilityTimer = 0.0f;
	
};

