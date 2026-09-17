#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/World/Actor.h"

CommonUtilities::Matrix4f SceneComponent::GetWorldMatrix() const
{
	const auto local = myTransform.GetLocalMatrix();
	return GetOwner() ? local * GetOwner()->GetTransform().GetWorldMatrix() : local;
}

CommonUtilities::Vector3f SceneComponent::GetWorldPosition() const
{
	const auto matrix = GetWorldMatrix();
	return {matrix(4, 1), matrix(4, 2), matrix(4, 3)};
}

CommonUtilities::Vector3f SceneComponent::GetWorldDirection() const
{
	const auto matrix = GetWorldMatrix();
	const CommonUtilities::Vector3f forward{matrix(3, 1), matrix(3, 2), matrix(3, 3)};
	return forward.LengthSqr() > 1e-8f ? forward.GetNormalized() : CommonUtilities::Vector3f::UnitZ;
}
