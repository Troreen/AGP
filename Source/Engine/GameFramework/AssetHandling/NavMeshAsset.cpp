#include "GameFramework.pch.h"
#include "NavMeshAsset.h"
#include <Logger/Logger.h>
#include <Importer.h>
#include <utility>

DECLARE_LOG_CATEGORY_WITH_NAME(NavMeshAssetLog, ASSET, Verbose);
DEFINE_LOG_CATEGORY(NavMeshAssetLog);

const std::vector<uint32_t>& NavMeshAsset::GetVertexIndices() const
{
	return myIndices;
}

const std::vector<CommonUtilities::Vector3f>& NavMeshAsset::GetVertices() const
{
	return myVertices;
}

std::vector<uint32_t>&& NavMeshAsset::MoveVertexIndices()
{
	return std::move(myIndices);
}

std::vector<CommonUtilities::Vector3f>&& NavMeshAsset::MoveVertices()
{
	return std::move(myVertices);
}

bool NavMeshAsset::Load(const std::filesystem::path& aPath, [[maybe_unused]] AssetRegistry& aRegistry)
{
	TGA::FBX::NavMesh importedNavMesh;
	TGA::FBX::Importer::InitImporter();

	if (!TGA::FBX::Importer::LoadNavMesh(aPath, importedNavMesh, true))
	{
		LOG(NavMeshAssetLog, Warning, "Could not load FBX navmesh '{}': {}", aPath.string(), TGA::FBX::Importer::GetLastSDKError());
		return false;
	}

	if (importedNavMesh.Chunks.empty())
	{
		LOG(NavMeshAssetLog, Warning, "Could not load FBX navmesh '{}': imported mesh had no chunks.", aPath.string());
		return false;
	}

	TGA::FBX::NavMesh::NavMeshChunk& navMeshChunk = importedNavMesh.Chunks.front();

	for (const TGA::FBX::NavMesh::NavMeshPolygon& polygon : navMeshChunk.Polygons)
	{
		for (uint32_t index : polygon.Indices)
		{
			myIndices.push_back(index);
		}
	}

	for (const TGA::FBX::Vertex& vertex : navMeshChunk.Vertices)
	{
		myVertices.emplace_back(GetSwizzledPosition(vertex));
	}

	return true;
}

CommonUtilities::Vector3f NavMeshAsset::GetSwizzledPosition(const TGA::FBX::Vertex& aTgaFbxVertex) const
{
	return CommonUtilities::Vector3f{ -aTgaFbxVertex.Position[1], aTgaFbxVertex.Position[0], -aTgaFbxVertex.Position[2] };
}
