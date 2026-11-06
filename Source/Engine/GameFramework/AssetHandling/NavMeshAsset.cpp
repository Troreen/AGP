#include "GameFramework.pch.h"
#include "NavMeshAsset.h"

#include <Logger/Logger.h>
#include <Importer.h>

#include <vector>
#include <unordered_map>
#include <map>
#include <utility>
#include <limits>
#include <numeric>
#include <algorithm>
#include <cstdint>
#include <cmath>

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

const CommonUtilities::Vector3f& NavMeshAsset::GetMinBounds() const
{
	return myMinBounds;
}

const CommonUtilities::Vector3f& NavMeshAsset::GetMaxBounds() const
{
	return myMaxBounds;
}

bool NavMeshAsset::Load(const std::filesystem::path& aPath, [[maybe_unused]] AssetRegistry& aRegistry)
{
	TGA::FBX::Importer::InitImporter();
	TGA::FBX::Mesh importedNavMesh;

	if (!TGA::FBX::Importer::LoadMesh(aPath, importedNavMesh))
	{
		LOG(NavMeshAssetLog, Warning, "Could not load FBX navmesh '{}': {}", aPath.string(), TGA::FBX::Importer::GetLastSDKError());
		return false;
	}

	if (!importedNavMesh.IsValid())
	{
		LOG(NavMeshAssetLog, Warning, "Could not load FBX navmesh '{}': imported mesh was not valid.", aPath.string());
		return false;
	}

	TGA::FBX::Mesh::Element& firstMeshElement = importedNavMesh.Elements.front();
	myIndices = std::move(firstMeshElement.Indices);

	std::vector<TGA::FBX::Vertex>& fbxVertices = firstMeshElement.Vertices;
	myVertices.reserve(fbxVertices.size());

	for (TGA::FBX::Vertex& fbxVertex : fbxVertices)
	{
		myVertices.emplace_back(GetSwizzledPosition(fbxVertex));
	}

	JoinVerticallyMisalignedVertices(myIndices, myVertices);
	JoinOverlappingBorderEdges(myIndices, myVertices);
	CullSmallerIslands(myIndices, myVertices);
	SwizzleMeshBounds(importedNavMesh.BoxBounds, myMinBounds, myMaxBounds);

	return true;
}

void NavMeshAsset::JoinVerticallyMisalignedVertices(std::vector<uint32_t>& outIndexList, std::vector<CommonUtilities::Vector3f>& outVertexList) const
{
	std::vector<CommonUtilities::Vector3f> verticallyMergedVertices;
	verticallyMergedVertices.reserve(outVertexList.size());

	std::vector<uint32_t> mergedIndexFromOldIndex(outVertexList.size());
	std::map<std::array<float, 2>, std::vector<uint32_t>> floorPointToMergedIndices;

	for (size_t vertexIndex = 0; vertexIndex < outVertexList.size(); ++vertexIndex)
	{
		const CommonUtilities::Vector3f& vertexPos = outVertexList[vertexIndex];
		std::vector<uint32_t>& mergedIndicesOnThisPoint = floorPointToMergedIndices[{ vertexPos.x, vertexPos.z }];

		uint32_t mergedIndex = static_cast<uint32_t>(verticallyMergedVertices.size());

		for (const uint32_t candidateIndex : mergedIndicesOnThisPoint)
		{
			if (std::abs(verticallyMergedVertices[candidateIndex].y - vertexPos.y) <= myMaxStepHeight)
			{
				mergedIndex = candidateIndex;
				break;
			}
		}

		if (mergedIndex == verticallyMergedVertices.size())
		{
			verticallyMergedVertices.push_back(vertexPos);
			mergedIndicesOnThisPoint.push_back(mergedIndex);
		}

		mergedIndexFromOldIndex[vertexIndex] = mergedIndex;
	}

	for (uint32_t& index : outIndexList)
	{
		index = mergedIndexFromOldIndex[index];
	}

	outVertexList = std::move(verticallyMergedVertices);
}

void NavMeshAsset::JoinOverlappingBorderEdges(std::vector<uint32_t>& outIndexList, std::vector<CommonUtilities::Vector3f>& outVertexList) const
{
	std::unordered_map<uint64_t, uint32_t> edgeKeyToEdgeCount;

	for (size_t polyStartIndex = 0; polyStartIndex < outIndexList.size(); polyStartIndex += 3)
	{
		for (size_t cornerIndex = 0; cornerIndex < 3; ++cornerIndex)
		{
			const uint32_t indexA = outIndexList[polyStartIndex + cornerIndex];
			const uint32_t indexB = outIndexList[polyStartIndex + (cornerIndex + 1) % 3];

			const uint64_t edgeKey = (static_cast<uint64_t>((std::min)(indexA, indexB)) << 32) | (std::max)(indexA, indexB);

			++edgeKeyToEdgeCount[edgeKey];
		}
	}

	std::vector<uint32_t> borderVertexIndices;
	borderVertexIndices.reserve(outVertexList.size());

	{
		std::vector<bool> isBorderVertex(outVertexList.size(), false);

		for (const auto& [edgeKey, edgeCount] : edgeKeyToEdgeCount)
		{
			if (edgeCount == 1)
			{
				const uint32_t minIndex = static_cast<uint32_t>(edgeKey >> 32);
				const uint32_t maxIndex = static_cast<uint32_t>(edgeKey & 0xFFFFFFFF);

				isBorderVertex[minIndex] = true;
				isBorderVertex[maxIndex] = true;
			}
		}

		for (size_t vertexIndex = 0; vertexIndex < outVertexList.size(); ++vertexIndex)
		{
			if (isBorderVertex[vertexIndex])
			{
				borderVertexIndices.push_back(static_cast<uint32_t>(vertexIndex));
			}
		}
	}

	struct VertexOnEdge
	{
		uint32_t vertexIndex;
		float alongEdgeRatio;

		bool operator < (const VertexOnEdge& anOther) const
		{
			return alongEdgeRatio < anOther.alongEdgeRatio;
		}
	};

	std::vector<uint32_t> boundaryIndices;
	std::vector<VertexOnEdge> verticesOnEdge;

	const size_t originalIndexCount = outIndexList.size();

	for (size_t polyStartIndex = 0; polyStartIndex < originalIndexCount; polyStartIndex += 3)
	{
		boundaryIndices.clear();

		uint32_t edgesWithVertices = 0;
		uint32_t cornerAcrossEdgeIndex = 0;

		for (size_t cornerIndex = 0; cornerIndex < 3; ++cornerIndex)
		{
			const uint32_t vertexIndexA = outIndexList[polyStartIndex + cornerIndex];
			const uint32_t vertexIndexB = outIndexList[polyStartIndex + (cornerIndex + 1) % 3];
			const uint64_t edgeKey = (static_cast<uint64_t>((std::min)(vertexIndexA, vertexIndexB)) << 32) | (std::max)(vertexIndexA, vertexIndexB);

			boundaryIndices.push_back(vertexIndexA);

			if (edgeKeyToEdgeCount.at(edgeKey) != 1)
			{
				continue;
			}

			const uint32_t oppositeCornerIndex = outIndexList[polyStartIndex + (cornerIndex + 2) % 3];
			const CommonUtilities::Vector3f& cornerPos = outVertexList[vertexIndexA];
			const CommonUtilities::Vector3f edgeVec = outVertexList[vertexIndexB] - cornerPos;
			const float edgeLenSqr = edgeVec.LengthSqr();

			verticesOnEdge.clear();

			for (const uint32_t borderVertexIndex : borderVertexIndices)
			{
				if (borderVertexIndex == oppositeCornerIndex)
				{
					continue;
				}

				const CommonUtilities::Vector3f cornerToBorderVec = outVertexList[borderVertexIndex] - cornerPos;

				const float borderVertAlongEdgeVec = cornerToBorderVec.Dot(edgeVec);

				if (borderVertAlongEdgeVec <= 0.0f || edgeLenSqr <= borderVertAlongEdgeVec)
				{
					continue;
				}

				const float borderVertBesideEdgeVec = cornerToBorderVec.Cross(edgeVec).y;

				if (myEdgeTolerance * myEdgeTolerance * edgeLenSqr <= borderVertBesideEdgeVec * borderVertBesideEdgeVec)
				{
					continue;
				}

				const float borderVertAboveEdgeVec = cornerToBorderVec.y * edgeLenSqr - edgeVec.y * borderVertAlongEdgeVec;

				if (myMaxStepHeight * edgeLenSqr < std::abs(borderVertAboveEdgeVec))
				{
					continue;
				}

				verticesOnEdge.emplace_back(VertexOnEdge{
					.vertexIndex = borderVertexIndex,
					.alongEdgeRatio = borderVertAlongEdgeVec
				});
			}

			if (verticesOnEdge.empty())
			{
				continue;
			}

			std::sort(verticesOnEdge.begin(), verticesOnEdge.end());

			for (const VertexOnEdge& vertexOnEdge : verticesOnEdge)
			{
				boundaryIndices.push_back(vertexOnEdge.vertexIndex);
			}

			++edgesWithVertices;
			cornerAcrossEdgeIndex = oppositeCornerIndex;
		}

		if (edgesWithVertices == 0)
		{
			continue;
		}

		uint32_t fanCenterIndex = 0;

		if (edgesWithVertices == 1)
		{
			fanCenterIndex = cornerAcrossEdgeIndex;
		}
		else
		{
			const CommonUtilities::Vector3f& cornerA = outVertexList[outIndexList[polyStartIndex]];
			const CommonUtilities::Vector3f& cornerB = outVertexList[outIndexList[polyStartIndex + 1]];
			const CommonUtilities::Vector3f& cornerC = outVertexList[outIndexList[polyStartIndex + 2]];

			const CommonUtilities::Vector3f polyCenter = (cornerA + cornerB + cornerC) / 3.0f;

			fanCenterIndex = static_cast<uint32_t>(outVertexList.size());
			outVertexList.push_back(polyCenter);
		}

		for (size_t boundaryIndexIndex = 0; boundaryIndexIndex < boundaryIndices.size(); ++boundaryIndexIndex)
		{
			const uint32_t fromIndex = boundaryIndices[boundaryIndexIndex];
			const uint32_t toIndex = boundaryIndices[(boundaryIndexIndex + 1) % boundaryIndices.size()];

			if (fromIndex == fanCenterIndex || toIndex == fanCenterIndex)
			{
				continue;
			}

			outIndexList.push_back(fromIndex);
			outIndexList.push_back(toIndex);
			outIndexList.push_back(fanCenterIndex);
		}

		const size_t lastSliceStartIndex = outIndexList.size() - 3;

		outIndexList[polyStartIndex] = outIndexList[lastSliceStartIndex];
		outIndexList[polyStartIndex + 1] = outIndexList[lastSliceStartIndex + 1];
		outIndexList[polyStartIndex + 2] = outIndexList[lastSliceStartIndex + 2];

		outIndexList.resize(lastSliceStartIndex);
	}
}

void NavMeshAsset::CullSmallerIslands(std::vector<uint32_t>& outIndexList, std::vector<CommonUtilities::Vector3f>& outVertexList) const
{
	// Create islands from polygons and find the densest island index.

	const uint32_t polyCount = static_cast<uint32_t>(outIndexList.size() / 3);
	std::vector<uint32_t> IslandIndexFromPolyIndex(polyCount);

	std::iota(IslandIndexFromPolyIndex.begin(), IslandIndexFromPolyIndex.end(), 0);

	uint32_t islandCount = polyCount;

	{
		std::unordered_map<uint64_t, uint32_t> edgeKeyToFirstPolyIndex;
		edgeKeyToFirstPolyIndex.reserve(outIndexList.size() / 2);

		for (uint32_t polyIndex = 0; polyIndex < polyCount; ++polyIndex)
		{
			const uint32_t indexIndex = polyIndex * 3;

			for (uint32_t cornerIndex = 0; cornerIndex < 3; ++cornerIndex)
			{
				const uint32_t nextCornerIndex = (1u << cornerIndex) & 3u;
				const uint32_t vertexIndexA = outIndexList[static_cast<size_t>(indexIndex + cornerIndex)];
				const uint32_t vertexIndexB = outIndexList[static_cast<size_t>(indexIndex + nextCornerIndex)];

				const uint32_t minVertexIndex = (std::min)(vertexIndexA, vertexIndexB);
				const uint32_t maxVertexIndex = (std::max)(vertexIndexA, vertexIndexB);

				const uint64_t edgeKey = (static_cast<uint64_t>(minVertexIndex) << 32) | maxVertexIndex;

				const auto [edgeIterator, isNewEdge] = edgeKeyToFirstPolyIndex.try_emplace(edgeKey, polyIndex);

				if (isNewEdge)
				{
					continue;
				}

				const uint32_t rootIndexA = GetRootIslandIndex(polyIndex, IslandIndexFromPolyIndex);
				const uint32_t rootIndexB = GetRootIslandIndex(edgeIterator->second, IslandIndexFromPolyIndex);

				if (rootIndexA == rootIndexB)
				{
					continue;
				}

				IslandIndexFromPolyIndex[(std::max)(rootIndexA, rootIndexB)] = (std::min)(rootIndexA, rootIndexB);
				--islandCount;
			}
		}
	}

	if (islandCount <= 1)
	{
		return;
	}

	std::vector<uint32_t> islandDensities(polyCount, 0);

	uint32_t winningRootIndex = 0;
	uint32_t largestIslandDensity = 0;

	for (uint32_t polyIndex = 0; polyIndex < polyCount; ++polyIndex)
	{
		const uint32_t rootIndex = GetRootIslandIndex(polyIndex, IslandIndexFromPolyIndex);

		uint32_t& islandDensity = islandDensities[rootIndex];
		++islandDensity;

		if (islandDensity > largestIslandDensity)
		{
			largestIslandDensity = islandDensity;
			winningRootIndex = rootIndex;
		}
	}

	// Build new index and vertex lists from the winning island's triangles only.

	constexpr uint32_t unusedVertexFlag = (std::numeric_limits<uint32_t>::max)();
	std::vector<uint32_t> oldToNewVertexIndex(outVertexList.size(), unusedVertexFlag);

	std::vector<uint32_t> islandIndices;
	islandIndices.reserve(outIndexList.size());

	std::vector<CommonUtilities::Vector3f> islandVertices;
	islandVertices.reserve(outVertexList.size());

	for (uint32_t polyIndex = 0; polyIndex < polyCount; ++polyIndex)
	{
		if (GetRootIslandIndex(polyIndex, IslandIndexFromPolyIndex) != winningRootIndex)
		{
			continue;
		}

		const uint32_t indexIndex = polyIndex * 3;

		for (uint32_t cornerIndex = 0; cornerIndex < 3; ++cornerIndex)
		{
			const uint32_t oldVertexIndex = outIndexList[static_cast<size_t>(indexIndex + cornerIndex)];

			if (oldToNewVertexIndex[oldVertexIndex] == unusedVertexFlag)
			{
				oldToNewVertexIndex[oldVertexIndex] = static_cast<uint32_t>(islandVertices.size());
				islandVertices.emplace_back(outVertexList[oldVertexIndex]);
			}

			islandIndices.push_back(oldToNewVertexIndex[oldVertexIndex]);
		}
	}

	outIndexList = std::move(islandIndices);
	outVertexList = std::move(islandVertices);
}

void NavMeshAsset::SwizzleMeshBounds(const TGA::FBX::Box& aTgaFbxBounds, CommonUtilities::Vector3f& outMinBounds, CommonUtilities::Vector3f& outMaxBounds) const
{
	outMinBounds.x = -aTgaFbxBounds.Max[1];
	outMinBounds.y = aTgaFbxBounds.Min[0];
	outMinBounds.z = -aTgaFbxBounds.Max[2];
	outMaxBounds.x = -aTgaFbxBounds.Min[1];
	outMaxBounds.y = aTgaFbxBounds.Max[0];
	outMaxBounds.z = -aTgaFbxBounds.Min[2];
}

CommonUtilities::Vector3f NavMeshAsset::GetSwizzledPosition(const TGA::FBX::Vertex& aTgaFbxVertex) const
{
	return CommonUtilities::Vector3f{ -aTgaFbxVertex.Position[1], aTgaFbxVertex.Position[0], -aTgaFbxVertex.Position[2] };
}

uint32_t NavMeshAsset::GetRootIslandIndex(uint32_t aPolyIndex, const std::vector<uint32_t>& anIslandIndexFromPolyIndexList) const
{
	uint32_t vertexIndex = anIslandIndexFromPolyIndexList[aPolyIndex];

	while (vertexIndex != anIslandIndexFromPolyIndexList[vertexIndex])
	{
		vertexIndex = anIslandIndexFromPolyIndexList[vertexIndex];
	}

	return vertexIndex;
}
