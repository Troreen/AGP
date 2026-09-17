#include "GameFramework/SceneComponent.h"
#include "GameFramework/World.h"
#include "World/TransformOperations.h"

CommonUtilities::Vector3f SceneComponent::GetWorldPosition() const
{
	auto matrix = GetWorldMatrix();
	return {matrix(4, 1), matrix(4, 2), matrix(4, 3)};
}

CommonUtilities::Vector3f SceneComponent::GetWorldDirection() const
{
	auto matrix = GetWorldMatrix();
	CommonUtilities::Vector3f direction{matrix(3, 1), matrix(3, 2), matrix(3, 3)};
	return direction.LengthSqr() > 1e-8f ? direction.GetNormalized() : CommonUtilities::Vector3f::UnitZ;
}

bool SceneComponent::SetParent(SceneComponent* parent, ReparentMode mode)
{
	GetWorld().EnsureMutationAllowed();
	if (!GetWorld().AcceptsChanges())
	{
		return false;
	}
	if (IsPendingDestroy() || (parent && (parent->GetOwner() != GetOwner() || parent->IsPendingDestroy())))
	{
		return false;
	}
	if (myAdmitted && parent && !parent->myAdmitted)
	{
		return false;
	}
	for (auto* p = parent; p; p = p->GetParent())
	{
		if (p == this)
		{
			return false;
		}
	}
	auto* parentValue = parent ? &parent->myTransform.myValue : &GetOwner()->GetTransform().myValue;
	if (!GameFrameworkInternal::ChangeParent(myTransform.myValue, parentValue, mode))
	{
		return false;
	}
	myParent = parent ? parent->GetRef<SceneComponent>() : ComponentRef<SceneComponent>{};
	return true;
}
