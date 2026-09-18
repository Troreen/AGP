#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/World/Actor.h"
#include "Maths.hpp"

CommonUtilities::Matrix4f SceneComponent::GetWorldMatrix() const
{
	const CommonUtilities::Matrix4f localMatrix = myTransform.GetLocalMatrix();
	return GetOwner() ? localMatrix * GetOwner()->GetTransform().GetWorldMatrix() : localMatrix;
}

CommonUtilities::Vector3f SceneComponent::GetWorldPosition() const
{
	const CommonUtilities::Matrix4f worldMatrix = GetWorldMatrix();
	return {worldMatrix(4, 1), worldMatrix(4, 2), worldMatrix(4, 3)};
}

CommonUtilities::Vector3f SceneComponent::GetWorldDirection() const
{
	const CommonUtilities::Matrix4f worldMatrix = GetWorldMatrix();
	const CommonUtilities::Vector3f forward{worldMatrix(3, 1), worldMatrix(3, 2), worldMatrix(3, 3)};
	return CU::NormalizeSafe(forward, CU::Vector3f::UnitZ);
}
