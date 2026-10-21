#pragma once


class Ability
{
public:

	Ability() = default;
	virtual ~Ability();

	virtual bool CanActivate() = 0;
	virtual void Activate() = 0;
	virtual void Update(float aDeltaTime);

protected:

	float myCooldown;
	float myCooldownRemaining;
	float myDuration;

private:

};

