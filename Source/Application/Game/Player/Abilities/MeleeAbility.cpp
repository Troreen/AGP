#include "MeleeAbility.h"
#include <GameFramework/Components/AnimatorComponent.h>

MeleeAbility::MeleeAbility(AnimatorComponent& anAnimator) : myAnimator(anAnimator)
{
    myCooldown = 0.5f;
}

bool MeleeAbility::CanActivate()
{
    return myCooldownRemaining <= 0.0f;
}

bool MeleeAbility::TryActivate(const AbilityActivationContext& aContext)
{
    aContext;

    if (!CanActivate())
    {
        return false;
    }

    if (myNextAttackIsFirst)
    {
        myAnimator.GetTree().SetTrigger("IsFirstMB1");
    }
    else
    {
        myAnimator.GetTree().SetTrigger("IsSecondMB1");
    }

    myNextAttackIsFirst = !myNextAttackIsFirst;

    myCooldownRemaining = myCooldown;

    return true;
}

void MeleeAbility::SetAttackHeld(bool aHeld)
{
    myAttackHeld = aHeld;
}

void MeleeAbility::Update(float aDeltaTime)
{
    Ability::Update(aDeltaTime);

    if (myAttackHeld && CanActivate())
    {
        AbilityActivationContext context = {};
        TryActivate(context);
    }
}
