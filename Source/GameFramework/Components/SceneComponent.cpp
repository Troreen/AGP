#include "SceneComponent.h"
#include "GameFramework/World/World.h"

CommonUtilities::Vector3f SceneComponent::GetWorldPosition() const
{
    auto matrix = GetWorldMatrix(); return {matrix(4,1),matrix(4,2),matrix(4,3)};
}
CommonUtilities::Vector3f SceneComponent::GetWorldDirection() const
{
    auto matrix = GetWorldMatrix();
    CommonUtilities::Vector3f direction{matrix(3,1),matrix(3,2),matrix(3,3)};
    return direction.LengthSqr() > 1e-8f ? direction.GetNormalized() : CommonUtilities::Vector3f::UnitZ;
}
bool SceneComponent::ApplyParent(SceneComponent* parent, ReparentMode mode)
{
    if (IsPendingDestroy() || (parent && (parent->GetOwner() != GetOwner() || parent->IsPendingDestroy()))) return false;
    for (auto* p = parent; p; p = p->GetParent()) if (p == this) return false;
    if (!GameFrameworkInternal::ChangeParent(myTransform, parent ? &parent->myTransform : &GetOwner()->GetLocalTransform(), mode)) return false;
    myParent = parent ? parent->GetHandle<SceneComponent>() : ComponentHandle<SceneComponent>{};
    return true;
}
bool SceneComponent::SetParent(SceneComponent* parent, ReparentMode mode)
{
    if (!GetWorld().AcceptsChanges() || IsPendingDestroy()) return false;
    if (parent && (parent->GetOwner() != GetOwner() || parent->IsPendingDestroy())) return false;
    for (auto* p = parent; p; p = p->GetParent()) if (p == this) return false;
    if (GetWorld().GetState() != World::State::Active) return ApplyParent(parent, mode);
    auto self = GetHandle<SceneComponent>();
    auto target = parent ? parent->GetHandle<SceneComponent>() : ComponentHandle<SceneComponent>{};
    GetWorld().QueueStructure([self, target, hasParent = parent != nullptr, mode](SceneDiagnostics& errors)
    {
        auto* object = self.Get(); if (!object) return;
        if ((hasParent && !target.Get()) || !object->ApplyParent(target.Get(), mode))
            errors.push_back({object->GetOwner()->GetName(),object->GetName(),"parent","Invalid component attachment"});
    });
    return true;
}
