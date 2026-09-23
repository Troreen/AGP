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
	Vertex ConvertVertex(const TGA::FBX::Vertex& aSourceVertex);
	bool HasImportedColor(const TGA::FBX::Vertex& aVertex);
	CU::Vector3f ConvertDirection(const float* aSourceVector, const CU::Vector3f& aFallback);
	Skeleton ConvertSkeleton(const TGA::FBX::Skeleton& aSourceSkeleton);
	CU::Matrix4f ConvertMatrix(const TGA::FBX::Matrix& aSourceMatrix);

	std::shared_ptr<Mesh> myMesh;
};
