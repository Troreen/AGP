#include "GameFramework/Components/MeshComponentBase.h"

#include "GraphicsEngine/Objects/Mesh.h"

#include <utility>

MeshComponentBase::MeshComponentBase(MeshAsset aMesh)
{
	SetMesh(std::move(aMesh));
}

void MeshComponentBase::SetMesh(MeshAsset aMesh)
{
	myMesh = std::move(aMesh.myResource);

	myMaterials.clear();
	if (myMesh != nullptr)
	{
		myMaterials.resize(myMesh->GetNumMaterialSlots());
	}

	OnMeshChanged();
}

MeshAsset MeshComponentBase::GetMesh() const
{
	MeshAsset result;
	result.myResource = myMesh;
	return result;
}

bool MeshComponentBase::HasMesh() const
{
	return myMesh != nullptr;
}

bool MeshComponentBase::SetMaterial(unsigned aMaterialIndex, MaterialAsset aMaterial)
{
	if (aMaterialIndex >= myMaterials.size() || !aMaterial)
	{
		return false;
	}
	myMaterials[aMaterialIndex] = std::move(aMaterial.myResource);
	return true;
}

MaterialAsset MeshComponentBase::GetMaterial(unsigned index) const
{
	MaterialAsset result;
	if (index < myMaterials.size())
	{
		result.myResource = myMaterials[index];
	}
	return result;
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

const std::array<CU::Matrix4f, 128>* MeshComponentBase::GetJointTransforms() const
{
	return nullptr;
}

void MeshComponentBase::OnMeshChanged()
{
}
