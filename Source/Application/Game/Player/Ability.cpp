#include "Ability.h"

Ability::~Ability()
{
}

void Ability::Update(float aDeltaTime)
{
	if (myCooldownRemaining > 0.0f)
	{
		myCooldownRemaining -= aDeltaTime;

		if (myCooldownRemaining < 0.0f)
		{
			myCooldownRemaining = 0.0f;
		}
	}
}
