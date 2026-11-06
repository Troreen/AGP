#pragma once
#include "Ability.h"

class AnimatorComponent;

class MeleeAbility final : public Ability
{
public:

    MeleeAbility() = delete;
    explicit MeleeAbility(AnimatorComponent& anAnimator);

    bool CanActivate() override;
    bool TryActivate(const AbilityActivationContext& aContext) override;

    void Update(float aDeltaTime) override;

    void SetAttackHeld(bool aHeld);

private:
    AnimatorComponent& myAnimator;

    bool myNextAttackIsFirst = true;
    bool myAttackHeld = false;
};
