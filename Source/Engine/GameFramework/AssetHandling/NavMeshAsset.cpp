#include "NavMeshAsset.h"
#include <Logger/Logger.h>
#include <unordered_map>
#include <unordered_set>

DECLARE_LOG_CATEGORY_WITH_NAME(NavMeshAssetLog, ASSET, Verbose);
DEFINE_LOG_CATEGORY(NavMeshAssetLog);

const std::vector<Pathfinder<float>::GraphNode>& NavMeshAsset::GetNavGraph() const
{
	return myNavGraph;
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

	std::unordered_map<uint64_t, std::vector<uint32_t>> edgeToPolyIndicesMap;
	std::unordered_map<uint32_t, std::vector<uint64_t>> polyIndexToEdgesMap;
	std::unordered_map<uint64_t, uint32_t> edgeToIndexMap;

	uint32_t currentEdgeIndex = 0;

	for (size_t i = 0; i < navMeshChunk.Polygons.size(); ++i)
	{
		const TGA::FBX::NavMesh::NavMeshPolygon& polygon = navMeshChunk.Polygons[i];

		for (size_t j = 0; j < polygon.Indices.size(); ++j)
		{
			const uint32_t indexA = polygon.Indices[j];
			const uint32_t indexB = polygon.Indices[(j + 1) % 3];

			const uint32_t low = (std::min)(indexA, indexB);
			const uint32_t high = (std::max)(indexA, indexB);

			const uint64_t edgeKey = (static_cast<uint64_t>(low) << 32) | high;
			const uint32_t polyIndex = static_cast<uint32_t>(i);

			edgeToPolyIndicesMap[edgeKey].push_back(polyIndex);
			polyIndexToEdgesMap[polyIndex].push_back(edgeKey);

			if (edgeToIndexMap.find(edgeKey) == edgeToIndexMap.cend())
			{
				edgeToIndexMap[edgeKey] = currentEdgeIndex;
				++currentEdgeIndex;
			}
		}
	}

	std::vector<Pathfinder<float>::GraphNode> graphNodes;
	graphNodes.resize(edgeToIndexMap.size());

	for (const auto& [edgeKey, polyIndices] : edgeToPolyIndicesMap)
	{
		const uint32_t vertexIndexA = static_cast<uint32_t>(edgeKey >> 32);
		const uint32_t vertexIndexB = static_cast<uint32_t>(edgeKey & 0xFFFFFFFF);

		const CommonUtilities::Vector3f vertexPosA = GetSwizzledPosition(navMeshChunk.Vertices[vertexIndexA]);
		const CommonUtilities::Vector3f vertexPosB = GetSwizzledPosition(navMeshChunk.Vertices[vertexIndexB]);

		Pathfinder<float>::GraphNode& node = graphNodes[edgeToIndexMap.at(edgeKey)];

		node.position = (vertexPosA + vertexPosB) * 0.5f;
		node.halfPortalVec = (vertexPosB - vertexPosA) * 0.5f;

		for (uint32_t polyIndex : polyIndices)
		{
			if (const auto& it = polyIndexToEdgesMap.find(polyIndex); it != polyIndexToEdgesMap.cend())
			{
				for (const uint64_t neighborEdgeKey : it->second)
				{
					if (neighborEdgeKey == edgeKey)
					{
						continue;
					}

					node.neighbors.emplace_back(Pathfinder<float>::GraphNode::Neighbor{
						.index = edgeToIndexMap.at(neighborEdgeKey),
						.girth = 0.0f // TODO
					});
				}
			}
		}
	}

	for (Pathfinder<float>::GraphNode& node : graphNodes)
	{
		for (Pathfinder<float>::GraphNode::Neighbor& neighbor : node.neighbors)
		{
			neighbor.cost = (graphNodes[neighbor.index].position - node.position).Length();
		}
	}

	myNavGraph = graphNodes;
}

CommonUtilities::Vector3f NavMeshAsset::GetSwizzledPosition(const TGA::FBX::Vertex& aTgaFbxVertex) const
{
	return CommonUtilities::Vector3f{ aTgaFbxVertex.Position[1], -aTgaFbxVertex.Position[2], aTgaFbxVertex.Position[0] };
}
