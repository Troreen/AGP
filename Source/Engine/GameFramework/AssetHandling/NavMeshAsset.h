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
		static constexpr float myMaxStepHeight = 35.0f;
		static constexpr float myEdgeTolerance = 2.0f;

		std::vector<uint32_t> myIndices;
		std::vector<CommonUtilities::Vector3f> myVertices;

		void JoinVerticallyMisalignedVertices(std::vector<uint32_t>& outIndexList, std::vector<CommonUtilities::Vector3f>& outVertexList) const;
		void JoinOverlappingBorderEdges(std::vector<uint32_t>& outIndexList, std::vector<CommonUtilities::Vector3f>& outVertexList) const;
		void CullSmallerIslands(std::vector<uint32_t>& outIndexList, std::vector<CommonUtilities::Vector3f>& outVertexList) const;

		CommonUtilities::Vector3f GetSwizzledPosition(const TGA::FBX::Vertex& aTgaFbxVertex) const;
		uint32_t GetRootIslandIndex(uint32_t aPolyIndex, const std::vector<uint32_t>& anIslandIndexFromPolyIndexList) const;
};
