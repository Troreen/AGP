#include "GameFramework/Components/MeshComponentBase.h"

#include "GraphicsEngine/Objects/Mesh.h"

#include <utility>

MeshComponentBase::MeshComponentBase(MeshHandle aMesh)
{
	SetMesh(std::move(aMesh));
}

void MeshComponentBase::SetMesh(MeshHandle aMesh)
{
	myMesh = aMesh.myResource;
	myMeshHandle = std::move(aMesh);

	myMaterials.clear();
	myMaterialHandles.clear();
	if (myMesh != nullptr)
	{
		myMaterials.resize(myMesh->GetNumMaterialSlots());
		myMaterialHandles.resize(myMesh->GetNumMaterialSlots());
	}

	OnMeshChanged();
}

void MeshComponentBase::SetMesh_DO_NOT_USE(std::shared_ptr<Mesh> aMesh)
{
	myMesh = aMesh;

	myMaterials.clear();
	myMaterialHandles.clear();
	if (myMesh != nullptr)
	{
		myMaterials.resize(myMesh->GetNumMaterialSlots());
		myMaterialHandles.resize(myMesh->GetNumMaterialSlots());
	}
}

MeshHandle MeshComponentBase::GetMesh() const
{
	return myMeshHandle;
}

bool MeshComponentBase::HasMesh() const
{
	return myMesh != nullptr;
}

bool MeshComponentBase::SetMaterial(unsigned aMaterialIndex, MaterialHandle aMaterial)
{
	if (aMaterialIndex >= myMaterials.size() || !aMaterial)
	{
		return false;
	}
	myMaterials[aMaterialIndex] = aMaterial.myResource;
	myMaterialHandles[aMaterialIndex] = std::move(aMaterial);
	return true;
}

MaterialHandle MeshComponentBase::GetMaterial(unsigned index) const
{
	return index < myMaterialHandles.size() ? myMaterialHandles[index] : MaterialHandle{};
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
