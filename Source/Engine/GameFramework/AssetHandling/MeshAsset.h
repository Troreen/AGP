#pragma once

#include "Asset.h"

#include <GraphicsEngine/Objects/Mesh.h>
#include <TgaFbxStructs.h>

class MeshAsset : public Asset
{
public:
	MeshAsset();
	explicit MeshAsset(const std::shared_ptr<Mesh>& aMesh);
	~MeshAsset() override;

	const std::shared_ptr<Mesh>& GetMesh() const;

protected:
	bool Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry) override;

private:
	void SetElements(const TGA::FBX::Mesh& aImportedMesh,
		std::vector<Mesh::Element>& inoutElements, std::vector<Vertex>& inoutVertices, std::vector<unsigned>& inoutIndices) const;
	void SetLandscapeElements(const TGA::FBX::Mesh& aImportedMesh,
		std::vector<Mesh::Element>& inoutElements, std::vector<Vertex>& inoutVertices, std::vector<unsigned>& inoutIndices) const;

	std::shared_ptr<Mesh> myMesh;
};
