#pragma once
#include "Asset.h"
#include <Vector3.hpp>
#include <TgaFbxStructs.h>
#include <cstdint>

class NavMeshAsset : public Asset
{
	public:
		const std::vector<uint32_t>& GetVertexIndices() const;
		const std::vector<CommonUtilities::Vector3f>& GetVertices() const;

		std::vector<uint32_t>&& MoveVertexIndices();
		std::vector<CommonUtilities::Vector3f>&& MoveVertices();

	protected:
		bool Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry) override;

	private:
		std::vector<uint32_t> myIndices;
		std::vector<CommonUtilities::Vector3f> myVertices;

		CommonUtilities::Vector3f GetSwizzledPosition(const TGA::FBX::Vertex& aTgaFbxVertex) const;
};
