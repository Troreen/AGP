#include "MeshComponentBase.h"

#include "Ensure.h"
#include "GraphicsEngine/Objects/Mesh.h"

#include <utility>

MeshComponentBase::MeshComponentBase(std::shared_ptr<Mesh> aMesh)
{
	SetMesh(std::move(aMesh));
}

void MeshComponentBase::SetMesh(std::shared_ptr<Mesh> aMesh)
{
	myMesh = std::move(aMesh);

	myMaterials.clear();
	if (myMesh != nullptr)
	{
		myMaterials.resize(myMesh->GetNumMaterialSlots());
	}

	OnMeshChanged();
}

std::shared_ptr<Mesh> MeshComponentBase::GetMesh() const
{
	return myMesh;
}

bool MeshComponentBase::HasMesh() const
{
	return myMesh != nullptr;
}

void MeshComponentBase::SetMaterial(unsigned aMaterialIndex, const std::shared_ptr<MaterialInterface>& aMaterial)
{
	ensure(aMaterialIndex < myMaterials.size());
	myMaterials[aMaterialIndex] = aMaterial;
}

void MeshComponentBase::SetVisible(bool aVisible)
{
	SetEnabled(aVisible);
}

bool MeshComponentBase::IsVisible() const
{
	return IsEnabled();
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
