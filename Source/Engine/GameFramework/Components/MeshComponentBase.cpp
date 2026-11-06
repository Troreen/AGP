#include "GameFramework.pch.h"
#include "GameFramework/Components/MeshComponentBase.h"

#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GraphicsEngine/Objects/Mesh.h"

#include <utility>

MeshComponentBase::MeshComponentBase(const std::shared_ptr<MeshAsset>& aMesh)
{
	SetMesh(aMesh);
}

void MeshComponentBase::SetMesh(const std::shared_ptr<MeshAsset>& aMesh)
{
	myMesh = aMesh;

	myMaterials.clear();
	if (myMesh != nullptr && myMesh->GetMesh() != nullptr)
	{
		myMaterials.resize(myMesh->GetMesh()->GetNumMaterialSlots());
	}

	OnMeshChanged();
}

const std::shared_ptr<MeshAsset> MeshComponentBase::GetMesh() const
{
	return myMesh;
}

bool MeshComponentBase::HasMesh() const
{
	return myMesh != nullptr;
}

bool MeshComponentBase::SetMaterial(unsigned aMaterialIndex, const std::shared_ptr<MaterialAsset>& aMaterial)
{
	if (aMaterialIndex >= myMaterials.size() || !aMaterial)
	{
		return false;
	}

	myMaterials[aMaterialIndex] = aMaterial;
	return true;
}

const std::shared_ptr<MaterialAsset> MeshComponentBase::GetMaterial(unsigned index) const
{
	return index < myMaterials.size() ? myMaterials[index] : nullptr;
}

CommonUtilities::Spheref MeshComponentBase::GetCullingSphere() const
{
	const std::shared_ptr<Mesh>& mesh = myMesh->GetMesh();

	CU::Vector3f boundsCenter;
	float boundsRadius;
	mesh->GetLocalBounds(boundsCenter, boundsRadius);

	const Transform& transform = GetOwner()->GetTransform();
	CU::Vector3f actorScale = transform.GetLocalScale();
	boundsRadius *= (std::max)(actorScale.x, (std::max)(actorScale.y, actorScale.z));

	const CU::Vector3f cullPosition = (boundsCenter.XYZ1() * GetOwner()->GetTransform().GetWorldMatrix()).XYZ();
	return CU::Spheref(cullPosition, boundsRadius);
}

void MeshComponentBase::SetVisible(bool aVisible)
{
	myVisible = aVisible;
}

bool MeshComponentBase::IsVisible() const
{
	return myVisible;
}

void MeshComponentBase::AddInstance(InstanceData&& aInstanceData)
{
	myInstanceData.emplace_back(std::move(aInstanceData));
}

const std::vector<InstanceData>& MeshComponentBase::GetInstanceData() const
{
	return myInstanceData;
}

bool MeshComponentBase::IsInstanced() const
{
	return !myInstanceData.empty();
}

bool MeshComponentBase::HasSkinning() const
{
	return false;
}

const std::array<CommonUtilities::Matrix4f, 128>* MeshComponentBase::GetJointTransforms() const
{
	return nullptr;
}

void MeshComponentBase::OnMeshChanged()
{
}
