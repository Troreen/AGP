#pragma once

#include "Component.h"
#include "Matrix.hpp"

#include <array>
#include <memory>
#include <vector>

#include "GraphicsEngine/Materials/MaterialInterface.h"

class Mesh;

class MeshComponentBase : public Component
{
public:
	MeshComponentBase() = default;
	explicit MeshComponentBase(std::shared_ptr<Mesh> aMesh);
	~MeshComponentBase() override = default;

	void SetMesh(std::shared_ptr<Mesh> aMesh);
	std::shared_ptr<Mesh> GetMesh() const;
	bool HasMesh() const;

	void SetMaterial(unsigned aMaterialIndex, const std::shared_ptr<MaterialInterface>& aMaterial);
	const std::vector<std::shared_ptr<MaterialInterface>>& GetMaterialList() const { return myMaterials; }

	void SetVisible(bool aVisible);
	bool IsVisible() const;

	virtual bool HasSkinning() const;
	virtual const std::array<CU::Matrix4f, 128>* GetJointTransforms() const;

protected:
	virtual void OnMeshChanged();

	std::shared_ptr<Mesh> myMesh;

private:
	std::vector<std::shared_ptr<MaterialInterface>> myMaterials;
};
