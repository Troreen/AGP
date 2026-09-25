#pragma once

#include "Asset.h"

#include <GraphicsEngine/Objects/Mesh.h>

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
	std::shared_ptr<Mesh> myMesh;
};
