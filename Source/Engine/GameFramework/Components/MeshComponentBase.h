#pragma once

#include "GameFramework/Components/SceneComponent.h"
#include "Matrix4x4.hpp"

#include <array>
#include <memory>
#include <vector>

class MaterialAsset;
class MeshAsset;
class SkeletalMeshComponent;

class WorldRenderer;

// Connects a spatial attachment to shared mesh/material assets. Component offsets
// compose with the owner; assets are shared by actors and render snapshots. Load and
// configure shared assets before play; 
// TODO: runtime asset editing needs a future safe API.
class MeshComponentBase : public SceneComponent
{
	friend class SkeletalMeshComponent;
	friend class WorldRenderer;

public:
	MeshComponentBase() = default;
	explicit MeshComponentBase(const std::shared_ptr<MeshAsset>& aMesh);
	~MeshComponentBase() override = default;

	// An empty binding clears the mesh. This never loads resources.
	void SetMesh(const std::shared_ptr<MeshAsset>& aMesh);
	const std::shared_ptr<MeshAsset>& GetMesh() const;
	bool HasMesh() const;

	// Invalid slots or empty materials leave the existing binding unchanged.
	bool SetMaterial(unsigned aMaterialIndex, const std::shared_ptr<MaterialAsset>& aMaterial);
	const std::shared_ptr<MaterialAsset>& GetMaterial(unsigned index) const;

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
	virtual const std::array<CommonUtilities::Matrix4f, 128>* GetJointTransforms() const;

	std::shared_ptr<MeshAsset> myMesh;
	std::vector<std::shared_ptr<MaterialAsset>> myMaterials;

	bool myVisible = true;
};
