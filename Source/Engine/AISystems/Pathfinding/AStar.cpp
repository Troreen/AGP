#include "AStar.h"
#include <Vector2.hpp>
#include <unordered_map>
#include <utility>
#include <cassert>
#include <cstdint>

void AStar::InitializeData(std::vector<uint32_t>& anIndexList, std::vector<CommonUtilities::Vector3f>& aVertexList, PersistentData& outGraphData)
{
	outGraphData.nodeIndicesFromPolyIndex.clear();
	outGraphData.graphNodes.clear();
	outGraphData.searchNodes.clear();
	outGraphData.heapNodes.Clear();
	outGraphData.searchCount = 0;

	std::unordered_map<uint64_t, std::vector<uint32_t>> edgeKeyToPolyIndicesMap;
	std::unordered_map<uint32_t, std::vector<uint64_t>> polyIndexToEdgeKeysMap;
	std::unordered_map<uint64_t, uint32_t> edgeKeyToEdgeIndexMap;

	uint32_t currentEdgeIndex = 0;

	for (size_t i = 0; i < anIndexList.size(); i += 3)
	{
		for (size_t j = 0; j < 3; ++j)
		{
			const uint32_t indexA = anIndexList[i + j];
			const uint32_t indexB = anIndexList[i + ((j + 1) % 3)];

			const uint32_t low = (std::min)(indexA, indexB);
			const uint32_t high = (std::max)(indexA, indexB);

			const uint64_t edgeKey = (static_cast<uint64_t>(low) << 32) | high;
			const uint32_t polyIndex = static_cast<uint32_t>(i / 3);
			
			edgeKeyToPolyIndicesMap[edgeKey].push_back(polyIndex);
			polyIndexToEdgeKeysMap[polyIndex].push_back(edgeKey);

			if (edgeKeyToEdgeIndexMap.find(edgeKey) == edgeKeyToEdgeIndexMap.cend())
			{
				edgeKeyToEdgeIndexMap[edgeKey] = currentEdgeIndex;
				++currentEdgeIndex;
			}
		}
	}

	{
		assert(edgeKeyToEdgeIndexMap.size() < AStar::invalidIndex && "AStar::InitializeData() generated a graph too large for 32-bit indices.");
		const size_t nodesCount = edgeKeyToEdgeIndexMap.size();

		outGraphData.nodeIndicesFromPolyIndex.assign(static_cast<uint32_t>(anIndexList.size() / 3), { invalidIndex, invalidIndex, invalidIndex });
		outGraphData.graphNodes.resize(nodesCount);
		outGraphData.searchNodes.resize(nodesCount);
		outGraphData.heapNodes.Reserve(nodesCount);
	}

	for (const auto& [edgeKey, polyIndices] : edgeKeyToPolyIndicesMap)
	{
		const uint32_t vertexIndexA = static_cast<uint32_t>(edgeKey >> 32);
		const uint32_t vertexIndexB = static_cast<uint32_t>(edgeKey & 0xFFFFFFFF);

		const CommonUtilities::Vector3f& vertexPosA = aVertexList[vertexIndexA];
		const CommonUtilities::Vector3f& vertexPosB = aVertexList[vertexIndexB];

		GraphNode& node = outGraphData.graphNodes[edgeKeyToEdgeIndexMap.at(edgeKey)];

		node.position = (vertexPosA + vertexPosB) * 0.5f;
		node.halfPortalVec = (vertexPosB - vertexPosA) * 0.5f;

		for (uint32_t polyIndex : polyIndices)
		{
			if (const auto& it = polyIndexToEdgeKeysMap.find(polyIndex); it != polyIndexToEdgeKeysMap.cend())
			{
				for (const uint64_t neighborEdgeKey : it->second)
				{
					if (neighborEdgeKey == edgeKey)
					{
						continue;
					}

					node.neighbors.emplace_back(GraphNode::Neighbor{
						.index = edgeKeyToEdgeIndexMap.at(neighborEdgeKey),
						.girth = 0.0f // TODO
					});
				}
			}
		}
	}

	for (const auto& [polyIndex, edgeKeys] : polyIndexToEdgeKeysMap)
	{
		const uint8_t maxIterations = static_cast<uint8_t>((std::min<size_t>)(edgeKeys.size(), 3));

		for (uint8_t i = 0; i < maxIterations; ++i)
		{
			outGraphData.nodeIndicesFromPolyIndex[polyIndex][i] = edgeKeyToEdgeIndexMap.at(edgeKeys[i]);
		}
	}

	for (GraphNode& node : outGraphData.graphNodes)
	{
		for (GraphNode::Neighbor& neighbor : node.neighbors)
		{
			neighbor.cost = (outGraphData.graphNodes[neighbor.index].position - node.position).Length();
		}
	}
}

bool AStar::Search(PersistentData& someGraphData, uint32_t aStartPolyIndex, uint32_t aTargetPolyIndex, const CommonUtilities::Vector3f& aStartPos, const CommonUtilities::Vector3f& aTargetPos, std::vector<uint32_t>& outNodeIndices)
{
	if (someGraphData.graphNodes.empty())
	{
		return false;
	}

	const uint32_t polyCount = static_cast<uint32_t>(someGraphData.nodeIndicesFromPolyIndex.size());

	assert(aStartPolyIndex < polyCount && "AStar::Search() was called with a start polygon index outside the graph.");
	assert(aTargetPolyIndex < polyCount && "AStar::Search() was called with a target polygon index outside the graph.");

	outNodeIndices.clear();
	someGraphData.heapNodes.Clear();

	if (aStartPolyIndex == aTargetPolyIndex)
	{
		return true;
	}

	const std::array<uint32_t, 3>& targetNodeIndices = someGraphData.nodeIndicesFromPolyIndex[aTargetPolyIndex];

	if (targetNodeIndices[0] == invalidIndex && targetNodeIndices[1] == invalidIndex && targetNodeIndices[2] == invalidIndex)
	{
		return false;
	}

	if (++someGraphData.searchCount == 0)
	{
		for (SearchNode& node : someGraphData.searchNodes)
		{
			node.searchStamp = 0;
		}

		someGraphData.searchCount = 1;
	}

	for (const uint32_t nodeIndex : someGraphData.nodeIndicesFromPolyIndex[aStartPolyIndex])
	{
		if (nodeIndex == invalidIndex)
		{
			continue;
		}

		const CommonUtilities::Vector3f& nodePos = someGraphData.graphNodes[nodeIndex].position;

		SearchNode& searchNode = detail::TouchSearchNode(someGraphData, nodeIndex);
		searchNode.bestTraversedDist = (nodePos - aStartPos).Length();
		searchNode.estimatedRemainingDist = (aTargetPos - nodePos).Length();
		searchNode.prevIndex = invalidIndex;

		someGraphData.heapNodes.Enqueue({
			.nodeIndex = nodeIndex,
			.targetDist = searchNode.estimatedRemainingDist,
			.totalDist = searchNode.bestTraversedDist + searchNode.estimatedRemainingDist
		});
	}

	uint32_t finalTargetNodeIndex = invalidIndex;

	while (0 < someGraphData.heapNodes.GetSize())
	{
		const HeapNode current = someGraphData.heapNodes.Dequeue();
		SearchNode& searchNode = someGraphData.searchNodes[current.nodeIndex];

		if (searchNode.visited)
		{
			continue;
		}

		searchNode.visited = true;

		if (current.nodeIndex == targetNodeIndices[0] || current.nodeIndex == targetNodeIndices[1] || current.nodeIndex == targetNodeIndices[2])
		{
			finalTargetNodeIndex = current.nodeIndex;
			break;
		}

		const float traversedDistance = searchNode.bestTraversedDist;

		for (const auto& neighbor : someGraphData.graphNodes[current.nodeIndex].neighbors)
		{
			assert(neighbor.index < someGraphData.graphNodes.size() && "AStar::Search() found a neighbor index outside of the graph.");

			SearchNode& neighborSearchNode = detail::TouchSearchNode(someGraphData, neighbor.index);

			if (neighborSearchNode.visited)
			{
				continue;
			}

			const float newTraversedDist = traversedDistance + neighbor.cost;

			if (newTraversedDist < neighborSearchNode.bestTraversedDist)
			{
				const bool isFirstDiscovery = neighborSearchNode.bestTraversedDist == (std::numeric_limits<float>::max)();

				if (isFirstDiscovery)
				{
					const CommonUtilities::Vector3f& neighborPosition = someGraphData.graphNodes[neighbor.index].position;
					neighborSearchNode.estimatedRemainingDist = (aTargetPos - neighborPosition).Length();
				}

				neighborSearchNode.bestTraversedDist = newTraversedDist;
				neighborSearchNode.prevIndex = current.nodeIndex;

				someGraphData.heapNodes.Enqueue({
					.nodeIndex = neighbor.index,
					.targetDist = neighborSearchNode.estimatedRemainingDist,
					.totalDist = newTraversedDist + neighborSearchNode.estimatedRemainingDist
				});
			}
		}
	}

	if (finalTargetNodeIndex == invalidIndex)
	{
		return false;
	}

	for (uint32_t nodeIndex = finalTargetNodeIndex; nodeIndex != invalidIndex; nodeIndex = someGraphData.searchNodes[nodeIndex].prevIndex)
	{
		outNodeIndices.push_back(nodeIndex);
	}

	return true;
}

void AStar::Funnel(PersistentData& someGraphData, const CommonUtilities::Vector3f& aStartPos, const CommonUtilities::Vector3f& aTargetPos, const std::vector<uint32_t>& somePathNodeIndices, std::vector<CommonUtilities::Vector3f>& outWaypoints)
{
	outWaypoints.clear();
	
	const size_t nodeCount = somePathNodeIndices.size();

	if (nodeCount == 0)
	{
		outWaypoints.push_back(aTargetPos);
		outWaypoints.push_back(aStartPos);
		return;
	}

	const size_t portalCount = nodeCount + 2;
	const size_t startPortalIndex = portalCount - 1;

	std::vector<CommonUtilities::Vector3f> wallPointsA(portalCount);
	std::vector<CommonUtilities::Vector3f> wallPointsB(portalCount);
	std::vector<CommonUtilities::Vector2f> flatWallPointsA(portalCount);
	std::vector<CommonUtilities::Vector2f> flatWallPointsB(portalCount);

	// Step 1: Unfold portal corridor into a flat strip

	{
		const GraphNode& firstNode = someGraphData.graphNodes[somePathNodeIndices[0]];

		wallPointsA[1] = firstNode.position - firstNode.halfPortalVec;
		wallPointsB[1] = firstNode.position + firstNode.halfPortalVec;
		flatWallPointsA[1] = CommonUtilities::Vector2f{ 0.0f, 0.0f };
		flatWallPointsB[1] = CommonUtilities::Vector2f{ (wallPointsB[1] - wallPointsA[1]).Length(), 0.0f };
	}

	for (size_t portalIndex = 0; portalIndex < portalCount; ++portalIndex)
	{
		if (portalIndex == 1)
		{
			continue;
		}

		const bool isTargetIndex = (portalIndex == 0);
		const bool isStartIndex = (portalIndex == startPortalIndex);

		const size_t neighborPortalIndex = isTargetIndex ? 1 : portalIndex - 1;

		CommonUtilities::Vector3f unsharedWallPoint = isTargetIndex ? aTargetPos : aStartPos;
		bool sharedPointIsOnWallA = false;

		if (!isTargetIndex && !isStartIndex)
		{
			const GraphNode& node = someGraphData.graphNodes[somePathNodeIndices[portalIndex - 1]];
			const CommonUtilities::Vector3f wallPointA = node.position - node.halfPortalVec;
			const CommonUtilities::Vector3f wallPointB = node.position + node.halfPortalVec;

			const CommonUtilities::Vector3f& prevWallA = wallPointsA[portalIndex - 1];
			const CommonUtilities::Vector3f& prevWallB = wallPointsB[portalIndex - 1];

			const float prevAToCurrentALenSqr = (wallPointA - prevWallA).LengthSqr();
			const float prevAToCurrentBLenSqr = (wallPointB - prevWallA).LengthSqr();
			const float prevBToCurrentALenSqr = (wallPointA - prevWallB).LengthSqr();
			const float prevBToCurrentBLenSqr = (wallPointB - prevWallB).LengthSqr();

			sharedPointIsOnWallA = (std::min)(prevAToCurrentALenSqr, prevAToCurrentBLenSqr) < (std::min)(prevBToCurrentALenSqr, prevBToCurrentBLenSqr);
			const bool sharedPointIsWallPointA = sharedPointIsOnWallA ? (prevAToCurrentALenSqr < prevAToCurrentBLenSqr) : (prevBToCurrentALenSqr < prevBToCurrentBLenSqr);

			unsharedWallPoint = sharedPointIsWallPointA ? wallPointB : wallPointA;
		}

		const CommonUtilities::Vector3f neighborPortalVec = wallPointsB[neighborPortalIndex] - wallPointsA[neighborPortalIndex];
		const CommonUtilities::Vector3f neighborWallAToUnsharedVec = unsharedWallPoint - wallPointsA[neighborPortalIndex];

		const float neighborPortalLenInv = 1.0f / neighborPortalVec.Length();
		const float distAlongNeighbor = neighborWallAToUnsharedVec.Dot(neighborPortalVec) * neighborPortalLenInv;
		const float distAheadOfNeighbor = neighborWallAToUnsharedVec.Cross(neighborPortalVec).Length() * neighborPortalLenInv * (isTargetIndex ? -1.0f : 1.0f);

		const CommonUtilities::Vector2f flatAlongNeighborDir = (flatWallPointsB[neighborPortalIndex] - flatWallPointsA[neighborPortalIndex]).GetNormalized();
		CommonUtilities::Vector2f flatAheadOfNeighborDir = { -flatAlongNeighborDir.y, flatAlongNeighborDir.x };

		if (flatAlongNeighborDir.Perp(flatAheadOfNeighborDir) < 0.0f)
		{
			flatAheadOfNeighborDir = -flatAheadOfNeighborDir;
		}

		const CommonUtilities::Vector2f flatUnsharedWallPoint = flatWallPointsA[neighborPortalIndex] + flatAlongNeighborDir * distAlongNeighbor + flatAheadOfNeighborDir * distAheadOfNeighbor;

		if (isTargetIndex || isStartIndex)
		{
			wallPointsA[portalIndex] = unsharedWallPoint;
			wallPointsB[portalIndex] = unsharedWallPoint;
			flatWallPointsA[portalIndex] = flatUnsharedWallPoint;
			flatWallPointsB[portalIndex] = flatUnsharedWallPoint;
		}
		else if (sharedPointIsOnWallA)
		{
			wallPointsA[portalIndex] = wallPointsA[portalIndex - 1];
			wallPointsB[portalIndex] = unsharedWallPoint;
			flatWallPointsA[portalIndex] = flatWallPointsA[portalIndex - 1];
			flatWallPointsB[portalIndex] = flatUnsharedWallPoint;
		}
		else
		{
			wallPointsA[portalIndex] = unsharedWallPoint;
			wallPointsB[portalIndex] = wallPointsB[portalIndex - 1];
			flatWallPointsA[portalIndex] = flatUnsharedWallPoint;
			flatWallPointsB[portalIndex] = flatWallPointsB[portalIndex - 1];
		}
	}

	// Step 2: Funneling in the flat strip

	std::vector<CommonUtilities::Vector3f> cornerPoints{ aTargetPos };
	std::vector<CommonUtilities::Vector2f> flatCornerPoints{ flatWallPointsA[0] };
	std::vector<size_t> cornerPortalIndices{ 0 };

	CommonUtilities::Vector2f flatFunnelApexPoint = flatWallPointsA[0];
	CommonUtilities::Vector2f flatFunnelPointA = flatFunnelApexPoint;
	CommonUtilities::Vector2f flatFunnelPointB = flatFunnelApexPoint;

	size_t funnelPortalIndexA = 0;
	size_t funnelPortalIndexB = 0;

	for (size_t portalIndex = 1; portalIndex < portalCount; ++portalIndex)
	{
		const CommonUtilities::Vector2f newArmVecA = flatWallPointsA[portalIndex] - flatFunnelApexPoint;
		const CommonUtilities::Vector2f newArmVecB = flatWallPointsB[portalIndex] - flatFunnelApexPoint;

		{
			const CommonUtilities::Vector2f armVecA = flatFunnelPointA - flatFunnelApexPoint;
			const CommonUtilities::Vector2f armVecB = flatFunnelPointB - flatFunnelApexPoint;

			const float perpTolerance = 1e-5f * (std::max)({ armVecA.LengthSqr(), armVecB.LengthSqr(), newArmVecB.LengthSqr() });

			if (-perpTolerance <= armVecB.Perp(newArmVecB))
			{
				if (armVecA.Perp(newArmVecB) <= perpTolerance)
				{
					flatFunnelPointB = flatWallPointsB[portalIndex];
					funnelPortalIndexB = portalIndex;
				}
				else
				{
					cornerPoints.push_back(wallPointsA[funnelPortalIndexA]);
					flatCornerPoints.push_back(flatFunnelPointA);
					cornerPortalIndices.push_back(funnelPortalIndexA);

					flatFunnelApexPoint = flatFunnelPointA;
					flatFunnelPointB = flatFunnelPointA;
					funnelPortalIndexB = funnelPortalIndexA;
					portalIndex = funnelPortalIndexA;
					continue;
				}
			}
		}

		{
			const CommonUtilities::Vector2f armVecA = flatFunnelPointA - flatFunnelApexPoint;
			const CommonUtilities::Vector2f armVecB = flatFunnelPointB - flatFunnelApexPoint;

			const float perpTolerance = 1e-5f * (std::max)({ armVecA.LengthSqr(), armVecB.LengthSqr(), newArmVecA.LengthSqr() });

			if (armVecA.Perp(newArmVecA) <= perpTolerance)
			{
				if (-perpTolerance <= armVecB.Perp(newArmVecA))
				{
					flatFunnelPointA = flatWallPointsA[portalIndex];
					funnelPortalIndexA = portalIndex;
				}
				else
				{
					cornerPoints.push_back(wallPointsB[funnelPortalIndexB]);
					flatCornerPoints.push_back(flatFunnelPointB);
					cornerPortalIndices.push_back(funnelPortalIndexB);

					flatFunnelApexPoint = flatFunnelPointB;
					flatFunnelPointA = flatFunnelPointB;
					funnelPortalIndexA = funnelPortalIndexB;
					portalIndex = funnelPortalIndexB;
					continue;
				}
			}
		}
	}

	cornerPoints.push_back(aStartPos);
	flatCornerPoints.push_back(flatWallPointsA[startPortalIndex]);
	cornerPortalIndices.push_back(startPortalIndex);

	// Step 3: Turn corner points into 3D waypoints and add extra waypoints to account for height differences along path.
	
	outWaypoints.push_back(aTargetPos);

	for (size_t cornerIndex = 1; cornerIndex < cornerPoints.size(); ++cornerIndex)
	{
		const CommonUtilities::Vector2f& flatCornerPoint = flatCornerPoints[cornerIndex - 1];
		const CommonUtilities::Vector2f flatPathVec = flatCornerPoints[cornerIndex] - flatCornerPoint;

		CommonUtilities::Vector3f lastCornerPolyNormal{ 0.0f, 0.0f, 0.0f };

		for (size_t portalIndex = cornerPortalIndices[cornerIndex - 1] + 1; portalIndex < cornerPortalIndices[cornerIndex]; ++portalIndex)
		{
			const CommonUtilities::Vector3f& wallPointA = wallPointsA[portalIndex];
			const CommonUtilities::Vector3f& wallPointB = wallPointsB[portalIndex];

			const CommonUtilities::Vector3f polyNormalBefore = (wallPointB - wallPointsA[portalIndex - 1]).Cross(wallPointA - wallPointsB[portalIndex - 1]);
			const CommonUtilities::Vector3f polyNormalAfter = (wallPointsB[portalIndex + 1] - wallPointA).Cross(wallPointsA[portalIndex + 1] - wallPointB);

			if (lastCornerPolyNormal.LengthSqr() == 0.0f)
			{
				lastCornerPolyNormal = polyNormalBefore;
			}

			if (lastCornerPolyNormal.LengthSqr() == 0.0f || polyNormalAfter.LengthSqr() == 0.0f)
			{
				continue; // No slope
			}

			const float pathDistSideA = flatPathVec.Perp(flatWallPointsA[portalIndex] - flatCornerPoint);
			const float pathDistSideB = flatPathVec.Perp(flatWallPointsB[portalIndex] - flatCornerPoint);

			if (std::abs(pathDistSideA - pathDistSideB) <= 1e-6f * (flatWallPointsB[portalIndex] - flatWallPointsA[portalIndex]).Length() * flatPathVec.Length())
			{
				continue; // Both sides of path run along the portal and not across
			}

			const float crossingRatioAToB = pathDistSideA / (pathDistSideA - pathDistSideB);
			CommonUtilities::Vector3f crossingPoint = wallPointA + (wallPointB - wallPointA) * crossingRatioAToB;

			if (crossingRatioAToB <= 1e-6f)
			{
				crossingPoint = wallPointA;
			}
			else if (1.0f - 1e-6f <= crossingRatioAToB)
			{
				crossingPoint = wallPointB;
			}

			const float lastCornerPointDistSqr = (crossingPoint - outWaypoints.back()).LengthSqr();
			const float slopeChangeSqr = (polyNormalAfter.GetNormalized() - lastCornerPolyNormal.GetNormalized()).LengthSqr();

			if (lastCornerPointDistSqr * slopeChangeSqr <= 1e-6f)
			{
				continue; // No or negligible slope
			}

			outWaypoints.push_back(crossingPoint);
			lastCornerPolyNormal = polyNormalAfter;
		}

		if (1e-6f < (cornerPoints[cornerIndex] - outWaypoints.back()).LengthSqr())
		{
			outWaypoints.push_back(cornerPoints[cornerIndex]);
		}
	}
}

//void AStar::SmoothWaypoints(float aRadius, uint8_t anIterationCount, const std::vector<CommonUtilities::Vector3f>& someInputWaypoints, std::vector<CommonUtilities::Vector3f>& outWaypoints)
//{
//	if (someInputWaypoints.size() < 3)
//	{
//		return;
//	}
//
//	outWaypoints.clear();
//	outWaypoints.push_back(someInputWaypoints[0]);
//
//	const size_t toIndex = someInputWaypoints.size() - 1;
//
//	for (size_t i = 1; i < toIndex; ++i)
//	{
//		const CommonUtilities::Vector3f& apexPoint = someInputWaypoints[i];
//		const CommonUtilities::Vector3f& prevPoint = someInputWaypoints[i - 1];
//		const CommonUtilities::Vector3f& nextPoint = someInputWaypoints[i + 1];
//
//		const CommonUtilities::Vector3f prevToApex = apexPoint - prevPoint;
//		const CommonUtilities::Vector3f apexToNext = nextPoint - apexPoint;
//
//		apexPoint - (apexPoint + (prevPoint - apexPoint).GetNormalized() * aRadius);
//
//		const CommonUtilities::Vector3f toApex = 
//
//		for (uint8_t j = 0; j < anIterationCount; ++j)
//		{
//			const CommonUtilities::Vector3f smoothPointA = prevToApex;
//			const CommonUtilities::Vector3f smoothPointB;
//		}
//
//	}
//
//	outWaypoints.push_back(someInputWaypoints.back());
//}

AStar::SearchNode& AStar::detail::TouchSearchNode(PersistentData& someGraphData, uint32_t aSearchNodeIndex)
{
	SearchNode& node = someGraphData.searchNodes[aSearchNodeIndex];

	if (node.searchStamp != someGraphData.searchCount)
	{
		node.searchStamp = someGraphData.searchCount;
		node.prevIndex = invalidIndex;
		node.bestTraversedDist = (std::numeric_limits<float>::max)();
		node.visited = false;
	}

	return node;
}
