#pragma once

#include "GameFramework/Components/SceneComponent.h"
#include "Matrix.hpp"

#include <array>
#include <memory>
#include <vector>

#include "GameFramework/Scenes/AssetRefs.h"

class Mesh;
class SkeletalMeshComponent;

class WorldRenderer;

// Connects a spatial attachment to shared mesh/material assets. Component offsets
// compose with the owner; assets are shared by actors and render snapshots. Load and
// configure shared assets before play; runtime asset editing needs a future safe API.
class MeshComponentBase : public SceneComponent
{
public:
	MeshComponentBase() = default;
	explicit MeshComponentBase(MeshAsset aMesh);
	~MeshComponentBase() override = default;

	// An empty binding clears the mesh. This never loads resources.
	void SetMesh(MeshAsset aMesh);
	MeshAsset GetMesh() const;
	bool HasMesh() const;

	// Invalid slots or empty materials leave the existing binding unchanged.
	bool SetMaterial(unsigned aMaterialIndex, MaterialAsset aMaterial);
	MaterialAsset GetMaterial(unsigned index) const;

	unsigned GetMaterialCount() const
	{
		return static_cast<unsigned>(myMaterials.size());
	}

	// Visibility affects extraction only; hidden skeletal meshes continue playback.
	void SetVisible(bool aVisible);
	bool IsVisible() const;

protected:
	virtual void OnMeshChanged();

private:
	virtual bool HasSkinning() const;
	virtual const std::array<CU::Matrix4f, 128>* GetJointTransforms() const;
	std::shared_ptr<Mesh> myMesh;
	std::vector<std::shared_ptr<MaterialInterface>> myMaterials;
	bool myVisible = true;
	friend class SkeletalMeshComponent;
	friend class WorldRenderer;
};
