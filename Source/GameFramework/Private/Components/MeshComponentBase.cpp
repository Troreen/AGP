#include "GameFramework/Components/MeshComponentBase.h"

#include "Runtime/Internal/AssetAccess.h"
#include "GraphicsEngine/Objects/Mesh.h"

#include <utility>

MeshComponentBase::MeshComponentBase(MeshAsset aMesh)
{
	SetMesh(std::move(aMesh));
}

void MeshComponentBase::SetMesh(MeshAsset aMesh)
{
    EnsureCanMutate();
	myMesh = GameFrameworkInternal::AssetAccess::Mesh(aMesh);

	myMaterials.clear();
	if (myMesh != nullptr)
	{
		myMaterials.resize(myMesh->GetNumMaterialSlots());
	}

	OnMeshChanged();
}

MeshAsset MeshComponentBase::GetMesh() const
{
	return GameFrameworkInternal::AssetAccess::WrapMesh(myMesh);
}

bool MeshComponentBase::HasMesh() const
{
	return myMesh != nullptr;
}

bool MeshComponentBase::SetMaterial(unsigned aMaterialIndex, MaterialAsset aMaterial)
{
    EnsureCanMutate();
    if (aMaterialIndex >= myMaterials.size() || !aMaterial) return false;
	myMaterials[aMaterialIndex] = GameFrameworkInternal::AssetAccess::Material(aMaterial);
    return true;
}
MaterialAsset MeshComponentBase::GetMaterial(unsigned index) const
{ return index < myMaterials.size() ? GameFrameworkInternal::AssetAccess::WrapMaterial(myMaterials[index]) : MaterialAsset{}; }

void MeshComponentBase::SetVisible(bool aVisible)
{
    EnsureCanMutate();
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
