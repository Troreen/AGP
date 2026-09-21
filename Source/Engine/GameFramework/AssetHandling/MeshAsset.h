#pragma once

#include "Asset.h"

#include <memory>

class Mesh;

class MeshAsset : public Asset
{
public:
	MeshAsset() = default;
	explicit MeshAsset(std::shared_ptr<Mesh> mesh) : myMesh(std::move(mesh)) {}
	const std::shared_ptr<Mesh>& GetMesh() const { return myMesh; }

protected:
	bool Load(const std::filesystem::path& path, AssetRegistry& registry) override;

private:
	std::shared_ptr<Mesh> myMesh;
};
