#include "GameFramework/Components/SkeletalMeshComponent.h"

#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GraphicsEngine/Objects/Mesh.h"

#include <utility>

SkeletalMeshComponent::SkeletalMeshComponent()
{
	OnMeshChanged();
}

SkeletalMeshComponent::SkeletalMeshComponent(const std::shared_ptr<MeshAsset>& aMesh) : MeshComponentBase(aMesh)
{
	OnMeshChanged();
}

void SkeletalMeshComponent::Update(float aDeltaTime)
{

}

bool SkeletalMeshComponent::HasSkinning() const
{
	return true;
}

const std::array<CommonUtilities::Matrix4f, 128>* SkeletalMeshComponent::GetJointTransforms() const
{
	return &myJointTransforms;
}


void SkeletalMeshComponent::OnMeshChanged()
{
	ResetJointTransforms();
}

void SkeletalMeshComponent::ResetJointTransforms()
{
	for (CommonUtilities::Matrix4f& transform : myJointTransforms)
	{
		transform = CommonUtilities::Matrix4f();
	}
}
