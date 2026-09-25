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

const std::shared_ptr<MeshAsset>& MeshComponentBase::GetMesh() const
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

const std::shared_ptr<MaterialAsset>& MeshComponentBase::GetMaterial(unsigned index) const
{
	return index < myMaterials.size() ? myMaterials[index] : nullptr;
}

void MeshComponentBase::SetVisible(bool aVisible)
{
	myVisible = aVisible;
}

bool MeshComponentBase::IsVisible() const
{
	return myVisible;
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
