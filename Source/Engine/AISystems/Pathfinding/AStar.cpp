#include "AStar.h"
#include <unordered_map>
#include <utility>
#include <cassert>
#include <cstdint>

void AStar::InitializeData(std::vector<uint32_t>& anIndexList, std::vector<Vector3f>& aVertexList, PersistentData& outData)
{
	outData.graphNodes.clear();
	outData.searchNodes.clear();
	outData.heapNodes.Clear();
	outData.searchCount = 0;

	std::unordered_map<uint64_t, std::vector<uint32_t>> edgeToPolyIndicesMap;
	std::unordered_map<uint32_t, std::vector<uint64_t>> polyIndexToEdgesMap;
	std::unordered_map<uint64_t, uint32_t> edgeToIndexMap;

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
			
			edgeToPolyIndicesMap[edgeKey].push_back(polyIndex);
			polyIndexToEdgesMap[polyIndex].push_back(edgeKey);

			if (edgeToIndexMap.find(edgeKey) == edgeToIndexMap.cend())
			{
				edgeToIndexMap[edgeKey] = currentEdgeIndex;
				++currentEdgeIndex;
			}
		}
	}

	{
		assert(edgeToIndexMap.size() < AStar::invalidIndex && "AStar::InitializeData() generated a graph too large for 32-bit indices.");
		const uint32_t nodesCount = edgeToIndexMap.size();

		outData.graphNodes.resize(nodesCount);
		outData.searchNodes.resize(nodesCount);
		outData.heapNodes.Reserve(nodesCount);
	}

	for (const auto& [edgeKey, polyIndices] : edgeToPolyIndicesMap)
	{
		const uint32_t vertexIndexA = static_cast<uint32_t>(edgeKey >> 32);
		const uint32_t vertexIndexB = static_cast<uint32_t>(edgeKey & 0xFFFFFFFF);

		const CommonUtilities::Vector3f& vertexPosA = aVertexList[vertexIndexA];
		const CommonUtilities::Vector3f& vertexPosB = aVertexList[vertexIndexB];

		GraphNode& node = outData.graphNodes[edgeToIndexMap.at(edgeKey)];

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

					node.neighbors.emplace_back(GraphNode::Neighbor{
						.index = edgeToIndexMap.at(neighborEdgeKey),
						.girth = 0.0f // TODO
					});
				}
			}
		}
	}

	for (GraphNode& node : outData.graphNodes)
	{
		for (GraphNode::Neighbor& neighbor : node.neighbors)
		{
			neighbor.cost = (outData.graphNodes[neighbor.index].position - node.position).Length();
		}
	}
}

void AStar::Search(PersistentData& data, uint32_t aStartIndex, uint32_t aTargetIndex, std::vector<uint32_t>& outPathIndices)
{
	assert(!data.graphNodes.empty() && "AStar::Search() was called with invalid data, no graph nodes exist.");

	const uint32_t graphSize = static_cast<uint32_t>(data.graphNodes.size());

	assert(aStartIndex < graphSize && "AStar::Search() was called with a start index outside the graph.");
	assert(aTargetIndex < graphSize && "AStar::Search() was called with a target index outside the graph.");

	outPathIndices.clear();
	data.heapNodes.Clear();

	if (++data.searchCount == 0)
	{
		for (SearchNode& node : data.searchNodes)
		{
			node.searchStamp = 0;
		}

		data.searchCount = 1;
	}

	if (aStartIndex == aTargetIndex)
	{
		outPathIndices.push_back(aTargetIndex);
		return;
	}

	const CommonUtilities::Vector3f& endPosition = data.graphNodes[aTargetIndex].position;
	detail::TouchSearchNode(data, aStartIndex).bestTraversedDist = 0.0f;

	data.heapNodes.Enqueue({
		.nodeIndex = aStartIndex,
		.targetDist = 0.0f,
		.totalDist = 0.0f
	});

	while (0 < data.heapNodes.GetSize())
	{
		const HeapNode current = data.heapNodes.Dequeue();
		SearchNode& searchNode = data.searchNodes[current.nodeIndex];

		if (searchNode.visited)
		{
			continue;
		}

		if (current.nodeIndex == aTargetIndex)
		{
			break;
		}

		searchNode.visited = true;
		const float traversedDistance = searchNode.bestTraversedDist;

		for (const auto& neighbor : data.graphNodes[current.nodeIndex].neighbors)
		{
			assert(neighbor.index < graphSize && "Pathfinder::AStar() found a neighbor index outside of the graph.");

			SearchNode& neighborSearchNode = detail::TouchSearchNode(data, neighbor.index);

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
					const CommonUtilities::Vector3f& neighborPosition = data.graphNodes[neighbor.index].position;
					neighborSearchNode.estimatedRemainingDist = (endPosition - neighborPosition).Length();
				}

				neighborSearchNode.bestTraversedDist = newTraversedDist;
				neighborSearchNode.prevIndex = current.nodeIndex;

				data.heapNodes.Enqueue({
					.nodeIndex = neighbor.index,
					.targetDist = neighborSearchNode.estimatedRemainingDist,
					.totalDist = newTraversedDist + neighborSearchNode.estimatedRemainingDist
				});
			}
		}
	}

	if (detail::TouchSearchNode(data, aTargetIndex).prevIndex == invalidIndex)
	{
		return;
	}

	for (uint32_t nodeIndex = aTargetIndex; nodeIndex != invalidIndex; nodeIndex = data.searchNodes[nodeIndex].prevIndex)
	{
		outPathIndices.push_back(nodeIndex);
	}
}

void AStar::Funnel(PersistentData& someData, const Vector3f& aStartPos, const Vector3f& aTargetPos, const std::vector<uint32_t>& pathIndices, std::vector<Vector3f>& outWaypoints)
{
	// TODO
	(void)someData;
	(void)aStartPos;
	(void)aTargetPos;
	(void)pathIndices;
	(void)outWaypoints;
}

AStar::SearchNode& AStar::detail::TouchSearchNode(PersistentData& someData, uint32_t aSearchNodeIndex)
{
	SearchNode& node = someData.searchNodes[aSearchNodeIndex];

	if (node.searchStamp != someData.searchCount)
	{
		node.searchStamp = someData.searchCount;
		node.prevIndex = invalidIndex;
		node.bestTraversedDist = (std::numeric_limits<float>::max)();
		node.visited = false;
	}

	return node;
}
